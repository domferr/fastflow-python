#ifndef PYTHON_UTILS
#define PYTHON_UTILS

#include <Python.h>

/* Returns the dictionary __main__.__dict__ */
inline PyObject* get_globals() {
    PyObject* main_mod_name = PyUnicode_FromString("__main__");
    auto main_module = PyImport_GetModule(main_mod_name);
    if (main_module == NULL) main_module = PyImport_Import(main_mod_name);
    Py_DECREF(main_mod_name);
    if (!main_module) return NULL;
    auto res = PyObject_GetAttrString(main_module, "__dict__");
    //Py_XINCREF(res);
    return res;
}

#endif // PYTHON_UTILS