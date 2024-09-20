#ifndef BASE_PROCESS
#define BASE_PROCESS

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>
#include <sys/wait.h>
#include "pickle.hpp"
#include "debugging.hpp"
#include "../py_ff_callback.hpp"
#include "messaging.hpp"
#include "object.h"
#include "../py_ff_constant.hpp"

#define SERIALIZED_EMPTY_TUPLE "(t."
#define SERIALIZED_NONE "N."

void process_body(int read_fd, int send_fd, bool isMultiOutput) {
    Message message;

    // Load pickling/unpickling functions
    pickling pickl;

    auto cleanup_exit = [&](){
        LOG("[child] cleanup");
        // Cleanup of objects created
        close(send_fd);
        close(read_fd);
        pickl.~pickling();
        // finalize python interpreter
        Py_Finalize();
        exit(0);
    };

    CHECK_ERROR_THEN("[child] load pickle/unpickle failure: ", cleanup_exit();)
    
    // receive serialized node
    int err = receiveMessage(read_fd, send_fd, message);
    if (err <= 0) handleError("[child] read serialized node", cleanup_exit());

    // deserialize Python node
    auto node = pickl.unpickle(message.data);
    CHECK_ERROR_THEN("[child] unpickle node failure: ", cleanup_exit();)

    py_ff_callback_object* callback = (py_ff_callback_object*) PyObject_CallObject(
        (PyObject *) &py_ff_callback_type, NULL
    );
    callback->ff_send_out_to_callback = [isMultiOutput, &pickl, read_fd, send_fd](PyObject* pydata, int index) {
        if (!isMultiOutput) {
            PyErr_SetString(PyExc_Exception, "Operation not available. This is not a multi output node");
            return (PyObject*) NULL;
        }

        if (index < 0) {
            PyErr_SetString(PyExc_Exception, "Index cannot be negative");
            return (PyObject*) NULL;
        }

        // we may have a fastflow constant as data to send out to index
        void *constant = NULL;
        std::string pydata_str = "";
        if (PyObject_TypeCheck(pydata, &py_ff_constant_type) != 0) {
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(pydata);
            constant = _const_result->ff_const;
        } else {
            // serialize pydata
            pydata_str = pickl.pickle(pydata);
            if (PyErr_Occurred()) return (PyObject*) NULL;
        }

        Message message;
        create_message_ff_send_out_to(message, index, constant, pydata_str);

        // send response
        int err = sendMessage(read_fd, send_fd, message);
        if (err <= 0) {
            PyErr_SetString(PyExc_Exception, "Unable to communicate ff_send_out_to request");
            return (PyObject*) NULL;
        }
        
        // receive response
        err = receiveMessage(read_fd, send_fd, message);
        if (err <= 0) {
            PyErr_SetString(PyExc_Exception, "Unable to get ff_send_out_to response");
            return (PyObject*) NULL;
        }

        return message.data.compare("t") == 0 ? Py_True:Py_False;
    };
    
    PyObject* main_module = PyImport_ImportModule("__main__");
    CHECK_ERROR_THEN("PyImport_ImportModule __main__ failure: ", cleanup_exit();)
    PyObject* globals = PyModule_GetDict(main_module);
    CHECK_ERROR_THEN("PyModule_GetDict failure: ", cleanup_exit();)
    
    // if you access the methods from the module itself, replace it with the callback
    if (PyDict_SetItemString(globals, "fastflow", (PyObject*) callback) == -1) {
        CHECK_ERROR_THEN("PyDict_SetItemString failure: ", cleanup_exit();)
    }
    // if you access the methods by importing them from the module, replace each method with the delegate's one
    if (PyDict_SetItemString(globals, "ff_send_out_to", PyObject_GetAttrString((PyObject*) callback, "ff_send_out_to")) == -1) {
        CHECK_ERROR_THEN("PyDict_SetItemString failure: ", cleanup_exit();)
    }

    while(err > 0) {
        err = receiveMessage(read_fd, send_fd, message);
        if (err <= 0) handleError("[child] read next", cleanup_exit());
        
        if (message.type == MESSAGE_TYPE_END_OF_LIFE) break;
        
        // deserialize data
        auto py_args_tuple = pickl.unpickle(message.data);
        CHECK_ERROR_THEN("[child] deserialize data failure: ", cleanup_exit();)
        // call function
        PyObject *py_func = PyObject_GetAttrString(node, message.f_name.c_str());
        CHECK_ERROR_THEN("[child] get node function: ", cleanup_exit();)

        if (py_func) {
            // finally call the function
            PyObject* py_result = PyTuple_Check(py_args_tuple) == 1 ? PyObject_CallObject(py_func, py_args_tuple):PyObject_CallFunctionObjArgs(py_func, py_args_tuple, nullptr);
            CHECK_ERROR_THEN("[child] call function failure: ", cleanup_exit();)

            // we may have a fastflow constant as result
            if (PyObject_TypeCheck(py_result, &py_ff_constant_type) != 0) {
                py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_result);
                err = sendMessage(read_fd, send_fd, { 
                    .type = _const_result->ff_const == ff::FF_EOS ? MESSAGE_TYPE_EOS:MESSAGE_TYPE_GO_ON
                });
                if (err <= 0) handleError("[child] send constant response", cleanup_exit());
            } else {
                std::string result_str = SERIALIZED_NONE;
                // serialize response
                if (py_result != Py_None) {
                    result_str = pickl.pickle(py_result);
                    CHECK_ERROR_THEN("[child] pickle result failure: ", cleanup_exit();)
                }

                // send response
                err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_RESPONSE, .data = result_str });
                if (err <= 0) handleError("[child] send response", cleanup_exit());

                Py_DECREF(py_result);
                Py_DECREF(py_func);
            }
        }
    }

    LOG("[child] after while");

    cleanup_exit();
}

class base_process {
public:    
    base_process(PyObject* node): node(node), send_fd(-1), read_fd(-1), registered_callback(NULL) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
    }
    
    int svc_init() {
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        // Hold the main GIL
        PyEval_RestoreThread(tstate);

        bool has_svc_init = PyObject_HasAttrString(node, "svc_init") == 1;
        has_svc_end = PyObject_HasAttrString(node, "svc_end") == 1;

        // Load pickling/unpickling functions
        pickling pickl;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)

        // serialize Python node to string
        auto node_str = pickl.pickle(node);
        CHECK_ERROR_THEN("pickle node failure: ", return -1;)

        int mainToChildFD[2]; // data to be sent from main process to child process
        int childToMainFD[2]; // data to be sent from child process to main process
        // create pipes
        if (pipe(mainToChildFD) == -1) {
            std::string msg = "Failed to create pipe. ";
            msg.append(strerror(errno));
            PyErr_SetString(PyExc_Exception, msg.c_str());
            return -1;
        }
        if (pipe(childToMainFD) == -1) {
            std::string msg = "Failed to create pipe. ";
            msg.append(strerror(errno));
            PyErr_SetString(PyExc_Exception, msg.c_str());
            return -1;
        }

        int returnValue = 0;
        // from cpython source code
        // https://github.com/python/cpython/blob/main/Modules/posixmodule.c#L8056
        PyOS_BeforeFork();
        pid = fork();
        if (pid == -1) {
            std::string msg = "Failed to fork. ";
            msg.append(strerror(errno));
            PyErr_SetString(PyExc_Exception, msg.c_str());
            returnValue = -1;
        } else if (pid == 0) { // child
            PyOS_AfterFork_Child();

            close(mainToChildFD[1]); // Close write end of mainToChildFD
            close(childToMainFD[0]); // Close read end of childToMainFD

            process_body(mainToChildFD[0], childToMainFD[1], registered_callback != NULL);
            std::string msg = "[child] shouldn't be here... ";
            msg.append(strerror(errno));
            PyErr_SetString(PyExc_Exception, msg.c_str());
            returnValue = -1;
        } else { // parent
            PyOS_AfterFork_Parent();

            // Release the main GIL
            PyEval_SaveThread();

            close(mainToChildFD[0]); // Close read end of mainToChildFD
            close(childToMainFD[1]); // Close write end of childToMainFD

            send_fd = mainToChildFD[1];
            read_fd = childToMainFD[0];

            // send serialized node
            int err = sendMessage(read_fd, send_fd, { .data = node_str });
            if (err <= 0) handleError("send serialized node", returnValue = -1);

            if (err > 0 && has_svc_init) {
                Message response;
                auto empty_tuple = std::string(SERIALIZED_EMPTY_TUPLE);
                int err = remote_procedure_call(send_fd, read_fd, empty_tuple, "svc_init", response);
                if (err <= 0) {
                    handleError("read result of remote call of svc_init", );
                } else {
                    // Hold the main GIL
                    PyEval_RestoreThread(tstate);
                    PyObject *svc_init_result = pickl.unpickle(response.data);
                    CHECK_ERROR_THEN("unpickle svc_init_result failure: ", returnValue = -1;)

                    returnValue = 0;
                    if (PyLong_Check(svc_init_result)) {
                        long res_as_long = PyLong_AsLong(svc_init_result);
                        returnValue = static_cast<int>(res_as_long);
                    }
                    Py_DECREF(svc_init_result);
                    pickl.~pickling();

                    // Release the main GIL
                    PyEval_SaveThread();
                }
            }
        }

        // from here the GIL is NOT acquired
        return returnValue;
    }

    void* svc(void *arg) {
        // arg may be equal to ff::FF_GO_ON in case of a node of a first set of an a2a that hasn't input channels
        std::string serialized_data = arg == NULL || arg == ff::FF_GO_ON ? SERIALIZED_EMPTY_TUPLE:*reinterpret_cast<std::string*>(arg);

        Message response;
        int err = remote_procedure_call(send_fd, read_fd, serialized_data, "svc", response);
        if (err <= 0) {
            handleError("remote call of svc", );
            return NULL;
        }

        while(response.type == MESSAGE_TYPE_REMOTE_PROCEDURE_CALL) {
            // the only supported remote procedure call from the child process
            // if the call of ff_send_out_to (as of today...)
            if (response.f_name.compare("ff_send_out_to") != 0) {
                handleError("got invalid f_name", );
                return NULL;
            }

            // parse received ff_send_out_to request
            int index;
            void *constant = NULL;
            std::string *data = new std::string();
            parse_message_ff_send_out_to(response, &constant, &index, data);

            // finally perform ff_send_out_to
            bool result = registered_callback->ff_send_out_to(constant != NULL ? constant:data, index);
            
            // send ff_send_out_to result
            err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_RESPONSE, .data = result ? "t":"f" });
            if (err <= 0) {
                handleError("error sending ff_send_out_to response", );
                return NULL;
            }

            // prepare for next iteration
            err = receiveMessage(read_fd, send_fd, response);
            if (err <= 0) {
                handleError("waiting for svc response", );
                return NULL;
            }
        }

        // got response of svc
        if (response.type == MESSAGE_TYPE_EOS) return ff::FF_EOS;
        if (response.type == MESSAGE_TYPE_GO_ON || response.data.compare(SERIALIZED_NONE) == 0) {
            return ff::FF_GO_ON;
        }

        return new std::string(response.data);
    }

    void svc_end() {
        if (has_svc_end) {
            Message response;
            auto empty_tuple = std::string(SERIALIZED_EMPTY_TUPLE);
            int err = remote_procedure_call(send_fd, read_fd, empty_tuple, "svc_end", response);
            if (err <= 0) handleError("read result of remote call of svc_end", );
        }

        // send end of life. Meanwhile we acquire the GIL and cleanup, the process will stop
        int err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_END_OF_LIFE });
        
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        // Cleanup of objects created
        Py_DECREF(node);
        node = nullptr;
        tstate = nullptr;
        // Release the main GIL
        PyEval_SaveThread();
        waitpid(pid, nullptr, 0);

        if (send_fd > 0) close(send_fd);
        send_fd = -1;
        if (read_fd > 0) close(read_fd);
        send_fd = -1;
    }

    void register_callback(ff::ff_monode* cb_node) {
        registered_callback = cb_node;
    }

    PyObject* get_python_object() {
        return node;
    }

private:
    PyThreadState *tstate;
    PyObject* node;
    bool has_svc_end;
    int send_fd;
    int read_fd;
    pid_t pid;
    ff::ff_monode* registered_callback;
};

#endif // BASE_PROCESS