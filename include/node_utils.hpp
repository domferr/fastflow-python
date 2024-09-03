#ifndef PY_FF_NODE_UTILS
#define PY_FF_NODE_UTILS

#include <Python.h>
#include <ff/ff.hpp>
#include "py_ff_node.hpp"
#include "py_ff_node_subint.hpp"
#include "py_ff_node_process.hpp"

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

ff::ff_node* args_to_node(PyObject *args, PyObject *kwds, bool use_subints) {
    PyObject *py_node = NULL;
    bool use_main_thread = false;
    if (parse_args(args, kwds, &py_node, &use_main_thread) == -1) return NULL;

    if (use_subints) {
        return new py_ff_node_subint(py_node);
    } else if (use_main_thread) {
        return new py_ff_node(py_node);
    }
    
    return new py_ff_node_process(py_node);
}

#endif // NODE_UTILS