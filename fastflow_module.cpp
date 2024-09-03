#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include <cstring>
#include <ostream>
#include <sstream>
#include "py_ff_a2a.hpp"
#include "py_ff_pipeline.hpp"
#include "py_ff_farm.hpp"

// A struct contains the definition of a module
PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "fastflow_module", // Module name
    "This is FastFlow's docstring",
    -1,   // Optional size of the module state memory
    NULL, // Optional module methods
    NULL, // Optional slot definitions
    NULL, // Optional traversal function
    NULL, // Optional clear function
    NULL  // Optional module deallocation function
};

PyMODINIT_FUNC
PyInit_fastflow_module(void) {
    PyObject* module = PyModule_Create(&moduledef);
    if (module == NULL)
        return NULL;
    
    // add FFPipeline
    if (PyType_Ready(&py_ff_pipeline_type) < 0)
        return NULL;
    
    if (PyModule_AddObject(module, "FFPipeline", (PyObject *) &py_ff_pipeline_type) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    
    // add FFFarm
    PyObject *py_ff_farm = PyType_FromSpec(&spec_py_ff_farm);
    if (py_ff_farm == NULL){
        return NULL;
    }
    Py_INCREF(py_ff_farm);
    
    if (PyModule_AddObject(module, "FFFarm", py_ff_farm) < 0){
        Py_DECREF(py_ff_farm);
        Py_DECREF(module);
        return NULL;
    }
    
    // add FFAllToAll
    if (PyType_Ready(&py_ff_a2a_type) < 0)
        return NULL;
    
    if (PyModule_AddObject(module, "FFAllToAll", (PyObject *) &py_ff_a2a_type) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    return module;
}

// Useful reading
// https://opensource.com/article/22/11/extend-c-python
// https://github.com/hANSIc99/PythonCppExtension