#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include <cstring>
#include <ostream>
#include <sstream>
#include <ff/ff.hpp>
#include "py_ff_a2a.hpp"
#include "py_ff_pipeline.hpp"
#include "py_ff_farm.hpp"
#include "py_ff_callback.hpp"
#include "process/ff_monode_process.hpp"
#include "process/ff_node_process.hpp"
#include "py_ff_constant.hpp"

PyDoc_STRVAR(ff_send_out_to_doc, "Send out to a node");

PyObject* empty_ff_send_out_to(PyObject *self, PyObject *args) {
    assert(self);
    
    PyErr_SetString(PyExc_Exception, "Operation not available. This is not a multi output node");
    return NULL;
}

/* initialization function */
static int
fastflow_module_exec(PyObject *module) 
{    
    // add FFPipeline
    if (PyType_Ready(&py_ff_pipeline_type) < 0)
        return -1;
    
    if (PyModule_AddObject(module, "FFPipeline", (PyObject *) &py_ff_pipeline_type) < 0) {
        return -1;
    }
    
    // add FFFarm
    PyObject *py_ff_farm = PyType_FromSpec(&spec_py_ff_farm);
    if (py_ff_farm == NULL){
        return -1;
    }
    Py_INCREF(py_ff_farm);
    
    if (PyModule_AddObject(module, "FFFarm", py_ff_farm) < 0) {
        Py_DECREF(py_ff_farm);
        return -1;
    }
    
    // add FFAllToAll
    if (PyType_Ready(&py_ff_a2a_type) < 0)
        return -1;
    
    if (PyModule_AddObject(module, "FFAllToAll", (PyObject *) &py_ff_a2a_type) < 0) {
        return -1;
    }

    // add the callback object. Do not AddObject, 
    // to avoid the user instantiating it
    if (PyType_Ready(&py_ff_callback_type) < 0) {
        return -1;
    }

    // add fastflow's constant type. Do not AddObject, 
    // to avoid the user instantiating it
    if (PyType_Ready(&py_ff_constant_type) < 0) {
        return -1;
    }

    if (PyModule_AddObject(module, GO_ON_CONSTANT_NAME, build_py_ff_constant(ff::FF_GO_ON)) < 0) {
        return -1;
    }

    return 0;
}

static PyMethodDef module_methods[] = {
    { "ff_send_out_to", (PyCFunction) empty_ff_send_out_to, METH_VARARGS, ff_send_out_to_doc },
    {NULL, NULL} /* Sentinel */
};

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, (void*) fastflow_module_exec},
#if PY_MINOR_VERSION >= 12
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
    {0, NULL},
};

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "fastflow_module",
    .m_doc = "This is FastFlow's docstring",
    .m_size = 0,
    .m_methods = module_methods,
    .m_slots = module_slots,
};

PyMODINIT_FUNC
PyInit_fastflow_module(void) {
    return PyModuleDef_Init(&moduledef);
}

// Useful reading
// https://opensource.com/article/22/11/extend-c-python
// https://github.com/hANSIc99/PythonCppExtension