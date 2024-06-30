#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include <cstring>
#include <ostream>
#include <sstream>
#include "py_ff_pipeline.hpp"

// A struct contains the definition of a module
PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "fastflow_subint_module", // Module name
    "This is FastFlow's docstring",
    -1,   // Optional size of the module state memory
    NULL, // Optional module methods
    NULL, // Optional slot definitions
    NULL, // Optional traversal function
    NULL, // Optional clear function
    NULL  // Optional module deallocation function
};

PyMODINIT_FUNC
PyInit_fastflow_subint_module(void) {
    PyObject* module = PyModule_Create(&moduledef);
    
    PyObject *py_ff_pipeline = PyType_FromSpec(&spec_py_ff_pipeline);
    if (py_ff_pipeline == NULL){
        return NULL;
    }
    Py_INCREF(py_ff_pipeline);
    
    if(PyModule_AddObject(module, "FFPipeline", py_ff_pipeline) < 0){
        Py_DECREF(py_ff_pipeline);
        Py_DECREF(module);
        return NULL;
    }
    return module;
}

// Useful reading
// https://opensource.com/article/22/11/extend-c-python
// https://github.com/hANSIc99/PythonCppExtension