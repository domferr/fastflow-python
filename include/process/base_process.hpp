#ifndef BASE_PROCESS
#define BASE_PROCESS

#include <Python.h>
#include "object.h"
#include <ff/ff.hpp>
#include <iostream>
#include <sys/wait.h>
#include <typeinfo>
#include <unordered_map>
#include "pickle.hpp"
#include "debugging.hpp"
#include "messaging.hpp"
#include "python_utils.hpp"
#include "py_ff_callback.hpp"
#include "py_ff_constant.hpp"

#define handleError(msg, then) do { perror(msg); then; } while(0)

#define SERIALIZED_EMPTY_TUPLE "(t."
#define SERIALIZED_NONE "N."

void process_body(std::string &node_ser, int read_fd, int send_fd, bool isMultiOutput, bool hasSvcInit) {
    Messaging messaging{ send_fd, read_fd };
    Message message;

    auto cleanup_exit = [&](){
        LOG("[child] cleanup and exit");
        close(send_fd);
        close(read_fd);
        exit(0);
    };

    // Load pickling/unpickling functions
    pickling pickl;
    CHECK_ERROR_THEN("[child] load pickle/unpickle failure: ", cleanup_exit();)

    PyObject* node = pickl.unpickle(node_ser);

    // create the callback object
    py_ff_callback_object* callback = (py_ff_callback_object*) PyObject_CallObject(
        (PyObject *) &py_ff_callback_type, NULL
    );
    callback->ff_send_out_to_callback = [isMultiOutput, &pickl, &messaging](PyObject* pydata, int index) {
        if (!isMultiOutput) {
            PyErr_SetString(PyExc_Exception, "Operation not available. This is not a multi output node");
            return (PyObject*) NULL;
        }

        if (index < 0) {
            PyErr_SetString(PyExc_Exception, "Index cannot be negative");
            return (PyObject*) NULL;
        }

        Message response;
        if (PyObject_TypeCheck(pydata, &py_ff_constant_type) != 0) {
            // we may have a fastflow constant as data to send out to index
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(pydata);
            int err = messaging.call_remote(response, "ff_send_out_to", index, _const_result->ff_const);
            if (err <= 0)  {
                PyErr_SetString(PyExc_Exception, "Error occurred sending ff_send_out_to request");
                return (PyObject*) NULL;
            }
        } else {
            int err = messaging.call_remote(response, "ff_send_out_to", index, pickl.pickle(pydata));
            if (PyErr_Occurred()) return (PyObject*) NULL;
            if (err <= 0)  {
                PyErr_SetString(PyExc_Exception, "Error occurred sending ff_send_out_to request");
                return (PyObject*) NULL;
            }
        }

        auto result = messaging.parse_data<bool>(response.data);
        return std::get<0>(result) ? Py_True:Py_False;
    };
    
    PyObject* globals = get_globals();
    // if you access the methods from the module itself, replace it with the callback
    if (PyDict_SetItemString(globals, "fastflow", (PyObject*) callback) == -1) {
        CHECK_ERROR_THEN("PyDict_SetItemString failure: ", cleanup_exit();)
    }
    // if you access the methods by importing them from the module, replace each method with the delegate's one
    if (PyDict_SetItemString(globals, "ff_send_out_to", PyObject_GetAttrString((PyObject*) callback, "ff_send_out_to")) == -1) {
        CHECK_ERROR_THEN("PyDict_SetItemString failure: ", cleanup_exit();)
    }

    int err = 1;
    if (hasSvcInit) {
        int result = 0;
        PyObject *py_func = PyObject_GetAttrString(node, "svc_init");
        PyObject* svc_init_result = PyObject_CallObject(py_func, nullptr);
        if (PyLong_Check(svc_init_result)) {
            result = static_cast<int>(PyLong_AsLong(svc_init_result));
        }
        err = messaging.send_response(result);
        if (err <= 0) handleError("[child] send svc_init result", cleanup_exit());
    }

    while(err > 0) {
        err = messaging.recv_message(message);
        if (err <= 0) handleError("[child] recv remote call", cleanup_exit());
        if (message.type == MESSAGE_TYPE_END_OF_LIFE) break;
        
        // deserialize data
        auto py_args_tuple = pickl.unpickle(message.data[0]);
        CHECK_ERROR_THEN("[child] deserialize data failure: ", std::cout << message.data[0] << std::endl; cleanup_exit();)

        // get the function reference
        PyObject *py_func = PyObject_GetAttrString(node, message.f_name.c_str());
        Py_XINCREF(py_func);
        CHECK_ERROR_THEN("[child] get node function: ", cleanup_exit();)

        if (py_func) {
            // finally call the function. Use PyObject_CallObject if we have a tuple already, PyObject_CallFunctionObjArgs otherwise
            PyObject* py_result;
            if (PyTuple_Check(py_args_tuple) == 1) py_result = PyObject_CallObject(py_func, py_args_tuple);
            else py_result = PyObject_CallFunctionObjArgs(py_func, py_args_tuple, nullptr);
            CHECK_ERROR_THEN("[child] call function failure: ", cleanup_exit();)

            // we may have a fastflow constant as result
            if (PyObject_TypeCheck(py_result, &py_ff_constant_type) != 0) {
                py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_result);
                err = messaging.send_response(_const_result->ff_const);
                if (err <= 0) handleError("[child] send constant response", cleanup_exit());
            } else {
                // send response
                if (py_result == Py_None) { // map None to GO_ON
                    // send GO_ON
                    err = messaging.send_response(ff::FF_GO_ON);
                } else {
                    // send serialized response
                    err = messaging.send_response(pickl.pickle(py_result));
                    CHECK_ERROR_THEN("[child] pickle result failure: ", cleanup_exit();)
                    Py_DECREF(py_result);
                }
                if (err <= 0) handleError("[child] send response", cleanup_exit());

                Py_DECREF(py_func);
            }
        }
    }

    LOG("[child] after while");

    cleanup_exit();
}

class base_process {
public:    
    base_process(PyObject* node): node(node), messaging(-1, -1), registered_callback(NULL) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
        has_svc_init = PyObject_HasAttrString(node, "svc_init") == 1;
        has_svc_end = PyObject_HasAttrString(node, "svc_end") == 1;
        pickling pickl;
    }
    
    int svc_init() {
        TIMESTART(svc_init_start_time);
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        int mainToChildFD[2]; // data to be sent from main process to child process
        int childToMainFD[2]; // data to be sent from child process to main process
        // create pipes
        if (pipe(mainToChildFD) == -1) {
            PyErr_Format(PyExc_Exception, "Failed to create pipe. %s", strerror(errno));
            return -1;
        }
        if (pipe(childToMainFD) == -1) {
            PyErr_Format(PyExc_Exception, "Failed to create pipe. %s", strerror(errno));
            return -1;
        }

        // Hold the main GIL
        PyEval_RestoreThread(tstate);
   
        int returnValue = 0;
        pickling pickl;
        CHECK_ERROR_THEN("load pickle/unpickle failure: ", return -1;)
        auto node_ser = pickl.pickle(node);

        TIMESTART(svc_init_fork);
        // from cpython source code
        // https://github.com/python/cpython/blob/9d0a75269c6ae361b1ed5910c3b3424ed93b6f6d/Modules/posixmodule.c#L8044
        PyOS_BeforeFork();
        pid = fork();
        if (pid == -1) {
            PyErr_Format(PyExc_Exception, "Failed to fork. %s", strerror(errno));
            returnValue = -1;
        } else if (pid == 0) { // child
            PyOS_AfterFork_Child();

            close(mainToChildFD[1]); // Close write end of mainToChildFD
            close(childToMainFD[0]); // Close read end of childToMainFD

            pickl.~pickling();
            process_body(node_ser, mainToChildFD[0], childToMainFD[1], registered_callback != NULL, has_svc_init);
            PyErr_Format(PyExc_Exception, "[child] shouldn't be here... %s", strerror(errno));
            returnValue = -1;
        }

        // parent
        PyOS_AfterFork_Parent();
        pickl.~pickling();

        // Release the main GIL
        PyEval_SaveThread();

        close(mainToChildFD[0]); // Close read end of mainToChildFD
        close(childToMainFD[1]); // Close write end of childToMainFD

        messaging = { mainToChildFD[1], childToMainFD[0] };
        if (has_svc_init) {
            // if the node has svc_init, the child process will call it
            // wait for svc_init result
            Message response;
            int err = 1;
            auto des_tuple = messaging.recv_deserialized_message<int>(response, &err);
            if (err <= 0) {
                returnValue = -1;
                PyErr_Format(PyExc_Exception, "Failed to receive svc_init result. %s", strerror(errno));
            } else {
                returnValue = std::get<0>(des_tuple);
            }
        }

        LOGELAPSED("svc_init time ", svc_init_start_time);
        // from here the GIL is NOT acquired
        return returnValue;
    }

    void* svc(void *arg) {
        TIMESTART(svc_start_time);
        // arg may be equal to ff::FF_GO_ON in case of a node of a first set of an a2a that hasn't input channels
        std::string serialized_data = arg == NULL || arg == ff::FF_GO_ON ? SERIALIZED_EMPTY_TUPLE:*reinterpret_cast<std::string*>(arg);

        Message response;
        int err = messaging.call_remote(response, "svc", serialized_data);
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
            std::tuple<int, std::string> args = messaging.parse_data<int, std::string>(response.data);
            // try to deserialize to constant. If it results into NULL, then it is NOT a FastFlow's constant
            void* constant = deserialize<void*>(std::get<1>(args));
            // finally perform ff_send_out_to
            bool result = registered_callback->ff_send_out_to(constant == NULL ? (void*) new std::string(std::get<1>(args)):constant, std::get<0>(args));
            
            // send ff_send_out_to result
            err = messaging.send_response(result);
            if (err <= 0) {
                handleError("error sending ff_send_out_to response", );
                return NULL;
            }

            // prepare for next iteration
            err = messaging.recv_message(response);
            if (err <= 0) {
                handleError("waiting for svc response", );
                return NULL;
            }
        }

        // got response of svc
        void* constant = deserialize<void*>(response.data[0]);
        if (constant != NULL) return constant;

        LOGELAPSED("svc time ", svc_start_time);

        return new std::string(response.data[0]);
    }

    void svc_end() {
        TIMESTART(svc_end_start_time);
        if (has_svc_end) {
            Message response;
            int err = messaging.call_remote(response, "svc_end", SERIALIZED_EMPTY_TUPLE);
            if (err <= 0) handleError("read result of remote call of svc_end", );
        }

        // send end of life. Meanwhile we acquire the GIL and cleanup, the process will stop
        int err = messaging.eol();
        if (err <= 0) handleError("sending EOL", );
        
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        // Cleanup of objects created
        Py_DECREF(node);
        node = nullptr;
        tstate = nullptr;
        // Release the main GIL
        PyEval_SaveThread();
        waitpid(pid, nullptr, 0);

        messaging.closefds();
        LOGELAPSED("svc_end time ", svc_end_start_time);
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
    bool has_svc_init;
    Messaging messaging;
    pid_t pid;
    ff::ff_monode* registered_callback;
};

#endif // BASE_PROCESS