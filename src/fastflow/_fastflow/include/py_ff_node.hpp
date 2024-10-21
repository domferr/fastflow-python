#ifndef PY_FF_NODE
#define PY_FF_NODE

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>
#include "error_macros.hpp"
#include "py_ff_constant.hpp"

class py_ff_node: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;

    py_ff_node(PyObject* node): node(node), svc_func(nullptr) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        svc_func = PyObject_GetAttrString(node, "svc");
        CHECK_ERROR("py_ff_node failure: ")
    }

    int svc_init() override {
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        // Acquire the main GIL
        PyEval_RestoreThread(tstate);

        int returnValue = 0;
        if (PyObject_HasAttrString(node, "svc_init")) {
            PyObject* svc_init_func = PyObject_GetAttrString(node, "svc_init");
            if (svc_init_func) {
                PyObject* py_result = PyObject_CallFunctionObjArgs(svc_init_func, NULL);
                CHECK_ERROR_THEN("call svc_init function failure: ", returnValue = -1;)
                if (PyLong_Check(py_result)) {
                    long res_as_long = PyLong_AsLong(py_result);
                    returnValue = static_cast<int>(res_as_long);
                }
                if (py_result) Py_DECREF(py_result);
                Py_DECREF(svc_init_func);
            }
        }

        // Release the main GIL
        PyEval_SaveThread();

        return returnValue;
    }

    void * svc(void *arg) override {
        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        
        PyObject* py_args = reinterpret_cast<PyObject*>(arg);
        PyObject* py_result = PyObject_CallFunctionObjArgs(svc_func, py_args, NULL);
        CHECK_ERROR("PyObject_CallObject failure: ")

        // Release the main GIL
        PyEval_SaveThread();

        // if we have a FastFlow's constant, return it
        if (PyObject_TypeCheck(py_result, &py_ff_constant_type) != 0) {
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_result);
            return _const_result->ff_const;
        }

        // map None to GO_ON
        if (py_result == Py_None) {
            return ff::FF_GO_ON;
        }

        return (void*) py_result;
    }

    void svc_end() override {
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

        // Cleanup of objects created
        Py_DECREF(svc_func);
        Py_DECREF(node);
        svc_func = nullptr;
        node = nullptr;
        PyEval_SaveThread();
        /*// cleanup thread state with main interpreter
        PyThreadState_Clear(tstate);
        PyThreadState_Delete(tstate);*/
        tstate = nullptr;
    }

private:
    PyThreadState *tstate;
    PyObject* node;
    PyObject* svc_func;
};

#endif // PY_FF_NODE