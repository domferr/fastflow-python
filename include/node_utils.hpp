#ifndef PY_FF_NODE_UTILS
#define PY_FF_NODE_UTILS

#include <Python.h>
#include <ff/ff.hpp>
#include "py_ff_node.hpp"
#include "subint/ff_monode_subint.hpp"
#include "subint/ff_node_subint.hpp"
#include "process/ff_monode_process.hpp"
#include "process/ff_node_process.hpp"

int parse_args(PyObject *args, PyObject *kwds, PyObject **py_node, bool *use_main_thread) {
    *use_main_thread = false;
    *py_node = NULL;

    PyObject *py_use_main_thread = Py_False;
    static const char *kwlist[] = { "", "use_main_thread", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|$O", const_cast<char**>(kwlist), py_node, &py_use_main_thread)) {
        assert(PyErr_Occurred());
        PyErr_SetString(PyExc_RuntimeError, "Tuple and keywords parsing failed");
        return -1;
    }

    if (py_use_main_thread != nullptr && !PyBool_Check(py_use_main_thread)) {
        PyErr_Format(PyExc_TypeError, "A bool is required for 'use_main_thread' keyword (got type %s)",
                     Py_TYPE(py_use_main_thread)->tp_name);
        return -1;
    }

    *use_main_thread = PyObject_IsTrue(py_use_main_thread) == 1;

    return 0;
}

ff::ff_node* args_to_node(PyObject *args, PyObject *kwds, bool use_subints, bool multi_output = false) {
    PyObject *py_node = NULL;
    bool use_main_thread = false;
    if (parse_args(args, kwds, &py_node, &use_main_thread) == -1) return NULL;

    if (use_subints) {
        return multi_output ? (ff::ff_node*)new ff_monode_subint(py_node):new ff_node_subint(py_node);
    } else if (use_main_thread) {
        return new py_ff_node(py_node);
    }
    
    return multi_output ? (ff::ff_node*)new ff_monode_process(py_node):new ff_node_process(py_node);
}

template<typename T>
PyObject* run_and_wait_end(T *node, bool use_subinterpreters) {
    PyObject* globals = NULL;
    if (use_subinterpreters) {
        // Load pickling/unpickling functions but in the main interpreter
        pickling pickling_main;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return NULL;)
        
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
        CHECK_ERROR_THEN("PyRun_String failure: ", return NULL;)
        // Cleanup of objects created
        pickling_main.~pickling();
    }

    // Release GIL while waiting for thread.
    int val = 0;
    Py_BEGIN_ALLOW_THREADS
    val = node->run_and_wait_end();
    Py_END_ALLOW_THREADS

    if (use_subinterpreters) {
        // cleanup of the environment string to free memory
        PyRun_String(R"PY(
__ff_environment_string = ""
        )PY", Py_file_input, globals, NULL);
    }

    return PyLong_FromLong(val);
}

#endif // NODE_UTILS