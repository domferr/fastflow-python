#ifndef BASE_SUBINT
#define BASE_SUBINT

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

#if PY_MINOR_VERSION >= 12
class base_subint {
public:
    base_subint(PyObject* node, bool is_multi_output = true): node(node), svc_func(nullptr), pickl(nullptr) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
    }

    int svc_init() {
        TIMESTART(svc_init_start_time);

        // Hold the main GIL
        PyEval_RestoreThread(tstate);

        // Load pickling/unpickling functions but in the main interpreter
        pickling pickling_main;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)

        // serialize Python node to bytes
        auto node_ser = pickling_main.pickle(node);
        CHECK_ERROR_THEN("pickle node failure: ", return -1;)

        PyObject* globals = get_globals();
        PyObject* py_env_str = PyDict_GetItemString(globals, "__ff_environment_string");
        Py_INCREF(py_env_str);
        auto env_str = PyUnicode_AsUTF8(PyObject_Str(py_env_str));

        // Cleanup of objects created
        pickling_main.~pickling();

        // associate a new thread state with fastflow node's thread
        tstate = PyThreadState_New(tstate->interp);

        TIMESTART(svc_init_create_interpr);

        // Create a new sub-interpreter with its own GIL
        PyInterpreterConfig sub_interp_config = {
            .use_main_obmalloc = 0,
            .allow_fork = 0,
            .allow_exec = 0,
            .allow_threads = 0,
            .allow_daemon_threads = 0,
            .check_multi_interp_extensions = 1,
            .gil = PyInterpreterConfig_OWN_GIL,
        };
        // save thread state with main interpreter
        PyThreadState* cached_tstate = tstate;
        // create subinterpreter (and drop the main GIL and acquire the new GIL)
        PyStatus status = Py_NewInterpreterFromConfig(&tstate, &sub_interp_config);
        /* The new interpreter is now active in the current thread */
        if (PyStatus_Exception(status)) {
            PRINT_ERROR("Py_NewInterpreterFromConfig failure")
            // cleanup thread state with main interpreter
            PyThreadState_Clear(cached_tstate);
            PyThreadState_Delete(cached_tstate);
            return -1;
        }

        // cleanup thread state with main interpreter
        PyThreadState_Clear(cached_tstate);
        PyThreadState_Delete(cached_tstate);

        LOGELAPSED("svc_init_create_interpr time ", svc_init_create_interpr);

        // recreate the global declarations and imports
        PyRun_SimpleString(env_str);

        py_ff_callback_object* callback = (py_ff_callback_object*) PyObject_CallObject(
            (PyObject *) &py_ff_callback_type, NULL
        );
        callback->ff_send_out_to_callback = [this](PyObject* pydata, int index) {
            return this->py_ff_send_out_to(pydata, index);
        };
        
        int returnValue = 0;
        
        globals = get_globals();
        // if you access the methods from the module itself, replace it with the callback
        if (PyDict_SetItemString(globals, "fastflow", (PyObject*) callback) == -1) {
            CHECK_ERROR_THEN("PyDict_SetItemString failure: ", returnValue = -1;)
        }
        // if you access the methods by importing them from the module, replace each method with the callback's one
        if (PyDict_SetItemString(globals, "ff_send_out_to", PyObject_GetAttrString((PyObject*) callback, "ff_send_out_to")) == -1) {
            CHECK_ERROR_THEN("PyDict_SetItemString failure: ", returnValue = -1;)
        }

        // create copy of the constants GO_ON and EOS for this subinterpreter
        if (PyDict_SetItemString(globals, GO_ON_CONSTANT_NAME, build_py_ff_constant(ff::FF_GO_ON)) == -1) {
            CHECK_ERROR_THEN("PyDict_SetItemString failure: ", returnValue = -1;)
        }

        if (PyDict_SetItemString(globals, EOS_CONSTANT_NAME, build_py_ff_constant(ff::FF_EOS)) == -1) {
            CHECK_ERROR_THEN("PyDict_SetItemString failure: ", returnValue = -1;)
        }

        // Load pickling/unpickling functions IN THE NEW INTERPRETER
        pickl = new pickling();
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", returnValue = -1;)

        // deserialize Python node
        // overwrite the old node (from the main interpreter) with the new one (in this subinterpreter)
        node = pickl->unpickle(node_ser);
        CHECK_ERROR_THEN("unpickle node failure: ", returnValue = -1;)

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
                if (py_result) Py_DECREF(py_result);
                Py_DECREF(svc_init_func);
            }
        }

        if (returnValue != 0) cleanup();

        LOGELAPSED("svc_init time ", svc_init_start_time);
        return returnValue;
    }

    void * svc(void *arg) {
        TIMESTART(svc_start_time);
        // arg may be equal to ff::FF_GO_ON in case of a node of a first set of an a2a that hasn't input channels
        PyObject* pickled_bytes = arg == ff::FF_GO_ON || arg == NULL ? NULL:reinterpret_cast<PyObject*>(arg);
        
        PyObject* py_args = arg == ff::FF_GO_ON || arg == NULL ? nullptr:pickl->unpickle_bytes(pickled_bytes);
        CHECK_ERROR_THEN("unpickle serialized data failure: ", return NULL;)
        //if (pickled_bytes) Py_DECREF(pickled_bytes);
        
        PyObject* py_result = py_args != nullptr && PyTuple_Check(py_args) == 1 ? PyObject_CallObject(svc_func, py_args):PyObject_CallFunctionObjArgs(svc_func, py_args, nullptr);
        CHECK_ERROR_THEN("PyObject_CallObject failure: ", return NULL;)
        
        // map None to ff::FF_GO_ON
        // we have None also if the svc function returned nothing (e.g. void function in c++)
        if (py_result == Py_None) {
            Py_DECREF(py_result);
            return ff::FF_GO_ON;
        }

        // we may have a fastflow constant as result
        if (PyObject_TypeCheck(py_result, &py_ff_constant_type) != 0) {
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_result);
            return _const_result->ff_const;
        }

        auto pickled_result_bytes = pickl->pickle_bytes(py_result);
        CHECK_ERROR_THEN("pickle result failure: ", return NULL;)
        //Py_INCREF(pickled_result_bytes);
        
        LOGELAPSED("svc time ", svc_start_time);
        
        return (void*) pickled_result_bytes;
    }

    void cleanup() {
        // Cleanup of objects created
        Py_DECREF(svc_func);
        Py_DECREF(node);
        svc_func = nullptr;
        node = nullptr;
        pickl->~pickling();
        pickl = nullptr;
        // End the interpreter
        Py_EndInterpreter(tstate);
        tstate = nullptr;
    }

    void svc_end() {
        TIMESTART(svc_end_start_time);
        
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

        LOGELAPSED("svc_end time ", svc_end_start_time);
    }

    PyObject* py_ff_send_out_to(PyObject *py_data, int index) {        
        if (registered_callback == NULL) {
            PyErr_SetString(PyExc_Exception, "Operation not available. This is not a multi output node");
            return NULL;
        }

        if (index < 0) {
            PyErr_SetString(PyExc_Exception, "Index cannot be negative");
            return (PyObject*) NULL;
        }

        // we may have a fastflow constant as data to send out to index
        if (PyObject_TypeCheck(py_data, &py_ff_constant_type) != 0) {
            py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(py_data);
            return registered_callback->ff_send_out_to(_const_result->ff_const, index) ? Py_True:Py_False;
        }

        auto pickled_data_bytes = pickl->pickle_bytes(py_data);
        CHECK_ERROR_THEN("pickle send out data failure: ", return NULL;)

        return registered_callback->ff_send_out_to(pickled_data_bytes, index) ? Py_True:Py_False;
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
    pickling* pickl;
    ff::ff_monode* registered_callback;
};
#else
class base_subint {
public:
    base_subint(PyObject* node, bool unused = false) {}

    int svc_init() {
        throw "Subinterpreters not supported";
    }

    void * svc(void *arg) {
        throw "Subinterpreters not supported";
    }

    void svc_end() {
        throw "Subinterpreters not supported";
    }

    void register_callback(ff::ff_monode* cb_node) { }

    PyObject* get_python_object() { return NULL; }
};
#endif

#endif // BASE_SUBINT