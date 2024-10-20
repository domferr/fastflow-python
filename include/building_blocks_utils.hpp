#ifndef BUILDING_BLOCKS_UTILS_HPP
#define BUILDING_BLOCKS_UTILS_HPP

#include <Python.h>
#include <ff/ff.hpp>
#include "py_ff_constant.hpp"
#include "pickle.hpp"

struct forwarder_minode: ff::ff_minode {  
    void *svc(void *in) { return in; }
};

struct forwarder_monode: ff::ff_monode {  
    void *svc(void *in) { return in; }
};

template<typename T>
void run(T *node, bool use_subinterpreters) {
    PyObject* globals = NULL;
    if (use_subinterpreters) {
        // Load pickling/unpickling functions but in the main interpreter
        pickling pickling_main;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return;)
        
        globals = get_globals();
        
        // run code to compute global declarations, imports, etc...
        PyRun_String(R"PY(
glb = [[k,v] for k,v in globals().items() if not (k.startswith('__') and k.endswith('__'))]

import inspect
__ff_environment_string = ""
for [k, v] in glb:
    try:
        if inspect.ismodule(v):
            if v.__package__:
                __ff_environment_string += f"from {v.__package__} import {k}"
            else: # v.__package__ is empty the module and the package are the same
                __ff_environment_string += f"import {k}"
        elif inspect.isclass(v) or inspect.isfunction(v):
            __ff_environment_string += inspect.getsource(v)
        # else:
        #    __ff_environment_string += f"{k} = {v}"
        __ff_environment_string += "\n"
    except:
        pass
        )PY", Py_file_input, globals, NULL);
        CHECK_ERROR_THEN("PyRun_String failure: ", return;)
        // Cleanup of objects created
        pickling_main.~pickling();
    }
    node->run();
}

void run_accelerator(ff::ff_pipeline** accelerator, ff::ff_node* bb, bool use_subinterpreters) {
    // if it is the first time running, initialize the accelerator
    if (*accelerator == nullptr) {
        *accelerator = new ff::ff_pipeline(true);
        (*accelerator)->add_stage(new forwarder_monode(), true);
        (*accelerator)->add_stage(bb, false);
        (*accelerator)->add_stage(new forwarder_minode(), true);
    }
    // finally run the building block through the accelerator
    run(*accelerator, use_subinterpreters);
}

template<typename T>
PyObject* wait(T *node, bool use_subinterpreters) {
    int val = 0;
    // Release GIL while waiting for thread
    Py_BEGIN_ALLOW_THREADS
    val = node->wait();
    Py_END_ALLOW_THREADS

    if (use_subinterpreters) {
        auto globals = get_globals();
        // cleanup of the environment string to free memory
        PyRun_String(R"PY(
__ff_environment_string = ""
        )PY", Py_file_input, globals, NULL);
    }

    return PyLong_FromLong(val);
}

template<typename T>
PyObject* run_and_wait_end(T *node, bool use_subinterpreters) {
    run(node, use_subinterpreters);
    return wait(node, use_subinterpreters);
}

PyObject* submit(ff::ff_pipeline* accelerator, PyObject* arg) {
    if (accelerator == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot submit data. The building block is not running, or you are running it via run_and_wait_end(). Please call run() instead");
        return NULL;
    }

    void *data;
    // we may have a fastflow constant as data to submit (e.g EOS)
    if (PyObject_TypeCheck(arg, &py_ff_constant_type) != 0) {
        py_ff_constant_object* _const = reinterpret_cast<py_ff_constant_object*>(arg);
        data = _const->ff_const;
    } else {
        pickling pkl;
        std::string* serialized_arg;
        if (pkl.pickle_ptr(arg, &serialized_arg) < 0) return PyBool_FromLong(false);
        data = (void*) serialized_arg;
    }
    return PyBool_FromLong(accelerator->offload(data));
}

PyObject* collect_next(ff::ff_pipeline* accelerator) {
    PyObject* next = Py_None;

    std::string* result;
    bool has_loaded = false;
    Py_BEGIN_ALLOW_THREADS
    has_loaded = accelerator->load_result((void**) &result);
    Py_END_ALLOW_THREADS

    if (has_loaded) {
        pickling pkl;
        next = pkl.unpickle(*result);
        free(result);
    }

    return next;
}
#endif // BUILDING_BLOCKS_UTILS_HPP