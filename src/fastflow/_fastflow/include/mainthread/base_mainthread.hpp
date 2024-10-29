#ifndef BASE_MAIN_THREAD
#define BASE_MAIN_THREAD

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>
#include <chrono>
#include "error_macros.hpp"
#include "pickle.hpp"
#include "debugging.hpp"
#include "py_ff_callback.hpp"
#include "py_ff_constant.hpp"
#include "python_utils.hpp"

class base_mainthread {
public:
    base_mainthread(PyObject* node): node(node), svc_func(nullptr), pkl(nullptr), last_data_sent(nullptr) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
    }

    int svc_init() {
        // associate a new thread state with fastflow node's thread
        tstate = PyThreadState_New(tstate->interp);
        
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);

        // Load pickling/unpickling functions
        pkl = new pickling();
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)
        
        int returnValue = 0;

        svc_func = PyObject_GetAttrString(node, "svc");

        // finally run svc_init if the user's node has it
        if (PyObject_HasAttrString(node, "svc_init")) {
            PyObject* svc_init_func = PyObject_GetAttrString(node, "svc_init");
            if (svc_init_func) {
                PyObject* py_result = PyObject_CallFunctionObjArgs(svc_init_func, NULL);
                CHECK_ERROR_THEN("call svc_init function failure: ", returnValue = -1;)
                if (PyLong_Check(py_result)) {
                    long res_as_long = PyLong_AsLong(py_result);
                    returnValue = static_cast<int>(res_as_long);
                }
                Py_XDECREF(py_result);
                Py_DECREF(svc_init_func);
            }
        }

        if (returnValue != 0) {
            cleanup();
        } else {
            // Release the main GIL
            PyEval_SaveThread();
        }

        return returnValue;
    }

    void * svc(void *arg) {
        if (arg == this->last_data_sent) arg = nullptr;

        // Acquire the main GIL
        PyEval_RestoreThread(tstate);

        // reinterpret_cast<PyObject*>(arg);
        // arg may be equal to ff::FF_GO_ON in case of a node of a first set of an a2a that hasn't input channels
        std::string* serialized_data = arg == ff::FF_GO_ON || arg == NULL ? NULL:reinterpret_cast<std::string*>(arg);
        PyObject* py_args = arg == ff::FF_GO_ON || arg == NULL ? nullptr:pkl->unpickle(*serialized_data);
        CHECK_ERROR_THEN("unpickle serialized data failure: ", return NULL;)
        if (serialized_data) free(serialized_data);
        
        PyObject* py_result = py_args != nullptr && PyTuple_Check(py_args) == 1 ? PyObject_CallObject(svc_func, py_args):PyObject_CallFunctionObjArgs(svc_func, py_args, nullptr);
        CHECK_ERROR_THEN("PyObject_CallObject failure: ", return NULL;)
        Py_XDECREF(py_args);
        
        void *return_value = nullptr;
        
        // map None to ff::FF_GO_ON
        // we have None also if the svc function returned nothing (e.g. void function in c++)
        if (py_result == Py_None) {
            return_value = ff::FF_GO_ON;
        } else if (PyObject_TypeCheck(py_result, &py_ff_constant_type) != 0) {
            // we may have a fastflow constant as result
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_result);
            return_value = _const_result->ff_const;
        } else {
            std::string* pickled_result;
            int err = pkl->pickle_ptr(py_result, &pickled_result);
            if (err < 0) return ff::FF_EOS;
            CHECK_ERROR_THEN("pickle result failure: ", return ff::FF_EOS;)
            return_value = (void*) pickled_result;
            this->last_data_sent = pickled_result;
            Py_DECREF(py_result);
        }

        // Release the main GIL
        PyEval_SaveThread();

        return return_value;
    }

    void svc_end() {
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        
        if (PyObject_HasAttrString(node, "svc_end") == 1) {
            PyObject* svc_end_func = PyObject_GetAttrString(node, "svc_end");
            CHECK_ERROR("get svc_end function failure: ")
            if (svc_end_func) {
                PyObject_CallFunctionObjArgs(svc_end_func, NULL);
                CHECK_ERROR("call svc_end function failure: ")
                Py_DECREF(svc_end_func);
            }
        }
        
        cleanup();
    }

    void cleanup() {
        // Cleanup of objects created
        Py_DECREF(svc_func);
        Py_DECREF(node);
        svc_func = nullptr;
        node = nullptr;
        pkl->~pickling();
        pkl = nullptr;
        PyEval_SaveThread();

        /*// cleanup thread state with main interpreter
        PyThreadState_Clear(tstate);
        PyThreadState_Delete(tstate);*/
        tstate = nullptr;
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
    PyObject* svc_func;
    pickling* pkl;
    ff::ff_monode* registered_callback;
    void* last_data_sent;
};

#endif // BASE_MAIN_THREAD