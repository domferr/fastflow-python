#ifndef PYTHON_UTILS
#define PYTHON_UTILS

#include <Python.h>

/* This is the equavalent of Python's globals(). Return a dictionary representing the global variables,
   or __main__.__dict__ if there is no frame.*/
inline PyObject* get_globals() {
    // from Python 3.13 we have PyEval_GetFrameGlobals instead of PyEval_GetGlobals
#if PY_VERSION_HEX >= 0x030d0000 
    PyObject *p = PyEval_GetFrameGlobals();
#else
    PyObject *p = PyEval_GetGlobals();
#endif
    if (p) return p;

    auto main_module = PyImport_ImportModule("__main__");
    if (!main_module) return NULL;
    auto res = PyObject_GetAttrString(main_module, "__dict__");
    //Py_XINCREF(res);
    return res;
}

#endif // PYTHON_UTILS