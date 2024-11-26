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

class base_process {
public:    
    base_process(PyObject* node): node(node), messaging(-1, -1), registered_callback(NULL), is_leftmost(false), process(nullptr) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
        has_svc_init = PyObject_HasAttrString(node, "svc_init") == 1;
        has_svc_end = PyObject_HasAttrString(node, "svc_end") == 1;
        pickling pickl;
    }

    int run(bool gil_is_acquired, bool immediate_start = false) {
        if (process != nullptr) return 0;

        //auto multiprocessing_start = std::chrono::system_clock::now();
        if (!gil_is_acquired) {
            // Release the main GIL
            PyEval_RestoreThread(tstate);
        }

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
        
        PyObject *kwargs = PyDict_New();
        auto fastflow_mod_name = PyUnicode_FromString("fastflow");
        auto fastflow_module = PyImport_GetModule(fastflow_mod_name);
        if (fastflow_module == NULL) fastflow_module = PyImport_Import(fastflow_mod_name);
        auto start_fastflow_process = PyObject_GetAttrString(fastflow_module, "_start_fastflow_process");
        Py_DECREF(fastflow_mod_name);
        Py_DECREF(fastflow_module);
        auto args = Py_BuildValue("(OiiOO)", node, mainToChildFD[0], childToMainFD[1], PyBool_FromLong(registered_callback != NULL), PyBool_FromLong(immediate_start));
        process = PyObject_Call(start_fastflow_process, args, kwargs);
        if (process == NULL) return -1;
        
        Py_DECREF(kwargs);
        Py_DECREF(args);
        Py_DECREF(start_fastflow_process);

        messaging = { mainToChildFD[1], childToMainFD[0] };
        if (!gil_is_acquired) {
            // Release the main GIL
            PyEval_SaveThread();
        }
        //auto multiprocessing_end = std::chrono::system_clock::now();
        //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(multiprocessing_end - multiprocessing_start).count();
        
        return 0;
    }

    int svc_init() {
        TIMESTART(svc_init_start_time);
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        if (!this->has_svc_init) return 0;

        this->run(false, true);

        int returnValue = 0;
        // if the node has svc_init, the child process will call it
        // while we wait for svc_init result
        Message response;
        int err = 1;
        auto des_tuple = messaging.recv_deserialized_message<int>(response, &err);
        if (err <= 0) {
            returnValue = -1;
            // Hold the main GIL
            PyEval_RestoreThread(tstate);
            PyErr_Format(PyExc_Exception, "Failed to receive svc_init result. %s", strerror(errno));
            // Release the main GIL
            PyEval_SaveThread();
        } else {
            returnValue = std::get<0>(des_tuple);
        }
        return returnValue;
    }

    void* svc(void *arg) {
        if (process == nullptr) this->run(false, true);

        // in some circumstances the node may receive as input the last data it has sent.
        // it happens for example for nodes who doesn't have a previous node
        if (arg == NULL) this->is_leftmost = true; // argument is null if the node is the leftmost
        if (this->is_leftmost) arg = nullptr;

        TIMESTART(svc_start_time);
        // arg may be equal to ff::FF_GO_ON in case of a node of a first set of an a2a that hasn't input channels
        std::string* serialized_data = arg == NULL || arg == ff::FF_GO_ON ? nullptr:reinterpret_cast<std::string*>(arg);

        Message response;
        int err = serialized_data == nullptr ? 
            messaging.call_remote(response, "svc", SERIALIZED_EMPTY_TUPLE):
            messaging.call_remote(response, "svc", *serialized_data);
        if (err <= 0) {
            handleError("remote call of svc", );
            return NULL;
        }
        if (serialized_data) free(serialized_data);
        
        while(response.type == MESSAGE_TYPE_REMOTE_PROCEDURE_CALL) {
            // the only supported remote procedure call from the child process
            // if the call of ff_send_out (as of today...)
            if (response.f_name.compare("ff_send_out") != 0) {
                handleError("got invalid f_name", );
                return NULL;
            }

            // parse received ff_send_out request
            std::tuple<std::string, int> args = messaging.parse_data<std::string, int>(response.data);
            // try to deserialize to constant. If it results into NULL, then it is NOT a fastFlow's constant
            void* constant = deserialize<void*>(std::get<0>(args));
            // finally perform ff_send_out
            int index = std::get<1>(args);
            auto data = constant == NULL ? (void*) new std::string(std::get<0>(args)):constant;
            bool result = index >= 0 ? 
                registered_callback->ff_send_out_to(data, index):
                registered_callback->ff_send_out(data);
            
            // send ff_send_out result
            err = messaging.send_response(result);
            if (err <= 0) {
                handleError("error sending ff_send_out response", );
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
        if (process != nullptr) {
            if (has_svc_end) {
                Message response;
                int err = messaging.call_remote(response, "svc_end", SERIALIZED_EMPTY_TUPLE);
                if (err <= 0) handleError("read result of remote call of svc_end", );
            }

            // send end of life. Meanwhile we acquire the GIL and cleanup, the process will stop
            int err = messaging.eol();
            if (err <= 0) handleError("sending EOL", );
        }
        
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        // Cleanup of objects created
        Py_DECREF(node);
        node = nullptr;
        tstate = nullptr;

        if (process != nullptr) {
            //auto multiprocessing_start = std::chrono::system_clock::now();
            auto join_func = PyObject_GetAttrString(process, "join");
            // join the process
            PyObject_CallNoArgs(join_func);
            Py_DECREF(join_func);
            Py_DECREF(process);
            process = nullptr;
            //auto multiprocessing_end = std::chrono::system_clock::now();
        }
        // Release the main GIL
        PyEval_SaveThread();

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
    ff::ff_monode* registered_callback;
    bool is_leftmost;
    PyObject* process;
};

#endif // BASE_PROCESS