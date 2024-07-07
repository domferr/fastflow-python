#ifndef PY_FF_NODE_SUBINT
#define PY_FF_NODE_SUBINT

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>
#include <chrono>
#include "error_macros.hpp"
#include "pickle.hpp"

class py_ff_node_subint: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;

    py_ff_node_subint(PyObject* node): node(node), svc_func(nullptr), pickl(nullptr) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        svc_func = PyObject_GetAttrString(node, "svc");
        CHECK_ERROR("py_ff_node_subint failure: ")
    }

    int svc_init() override {
        auto svc_init_start_time = std::chrono::system_clock::now();
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        // Hold the main GIL
        PyEval_RestoreThread(tstate);

        // Load pickling/unpickling functions
        LOAD_PICKLE_UNPICKLE
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)

        // serialize Python node to string
        PICKLE_PYOBJECT(node_str, node);
        CHECK_ERROR_THEN("pickle node failure: ", return -1;)
        
        PyObject* main_module = PyImport_ImportModule("__main__");
        CHECK_ERROR_THEN("PyImport_ImportModule __main__ failure: ", return -1;)
        PyObject* globals = PyModule_GetDict(main_module);
        CHECK_ERROR_THEN("PyModule_GetDict failure: ", return -1;)
        
        // run code to compute global declarations, imports, etc...
        PyRun_String(R"PY(
glb = [[k,v] for k,v in globals().items() if not (k.startswith('__') and k.endswith('__'))]

import inspect
res = ""
for [k, v] in glb:
    try:
        if inspect.ismodule(v):
            res += f"import {k}"
        elif inspect.isclass(v) or inspect.isfunction(v):
            res += inspect.getsource(v)
        #else:
        #    res += f"{k} = {v}"
        res += "\n"
    except:
        pass
        )PY", Py_file_input, globals, NULL);
        CHECK_ERROR_THEN("PyRun_String failure: ", return -1;)

        PyObject* result = PyDict_GetItemString(globals, "res");
        std::string env_str = std::string(PyUnicode_AsUTF8(PyObject_Str(result)));
        
        // Cleanup of objects created
        Py_DECREF(result);
        //todo Py_DECREF(globals);
        UNLOAD_PICKLE_UNPICKLE

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
        cached_tstate = tstate;
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

        int returnValue = 0;
        // cleanup thread state with main interpreter
        PyThreadState_Clear(cached_tstate);
        PyThreadState_Delete(cached_tstate);
        
        // Load pickling/unpickling functions IN THE NEW INTERPRETER
        pickl = new pickling();
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", returnValue = -1;)

        // recreate the global declarations and imports
        PyRun_SimpleString(env_str.c_str());

        // deserialize Python node
        // overwrite the old node (from the main interpreter) with the new one (in this subinterpreter)
        node = pickl->unpickle(node_str);
        CHECK_ERROR_THEN("unpickle node failure: ", returnValue = -1;)

        svc_func = PyObject_GetAttrString(node, "svc");

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

        auto svc_init_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_init_start_time).count();
        std::cerr << "svc_init time " << svc_init_time_ms << "ms" << std::endl;
        return returnValue;
    }

    void * svc(void *arg) override {      
        auto svc_start_time = std::chrono::system_clock::now();  
        std::string* serialized_data = arg == NULL ? NULL:reinterpret_cast<std::string*>(arg);
        
        PyObject* py_args = arg == NULL ? PyTuple_New(0):pickl->unpickle(*serialized_data);
        CHECK_ERROR_THEN("unpickle serialized data failure: ", return NULL;)
        //todo if (serialized_data) free(serialized_data);
        PyObject* py_result = PyObject_CallObject(svc_func, py_args);//PyObject_CallFunctionObjArgs(svc_func, py_args, NULL);
        CHECK_ERROR_THEN("PyObject_CallObject failure: ", return NULL;)

        if (py_result == Py_None) {
            Py_DECREF(py_result);
            return NULL;
        }

        auto serialized_result = new std::string(pickl->pickle(py_args));
        CHECK_ERROR_THEN("pickle result failure: ", return NULL;)

        auto svc_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_start_time).count();
        std::cerr << "svc time " << svc_time_ms << "ms" << std::endl;
        return (void*) serialized_result;
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

    void svc_end() override {
        auto svc_end_start_time = std::chrono::system_clock::now();
        
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

        auto svc_end_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_end_start_time).count();
        std::cerr << "svc_end time " << svc_end_time_ms << "ms" << std::endl;
    }

private:
    PyThreadState *tstate;
    PyObject* node;
    PyObject* svc_func;
    pickling* pickl;
};

#endif // PY_FF_NODE_SUBINT