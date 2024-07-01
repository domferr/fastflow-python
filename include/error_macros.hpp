#ifndef ERROR_MACROS
#define ERROR_MACROS

// workaround to suppress vscode warnings
extern PyObject* PyErr_GetRaisedException(void);

#define PRINT_ERROR(descr) \
{ \  
    PyObject* err = PyErr_GetRaisedException(); \
    PyObject* err_str = PyObject_Repr(err); \
    std::cerr << descr << PyUnicode_AsUTF8(err_str) << std::endl;   \
    Py_DECREF(err_str);     \
    Py_DECREF(err);     \
    PyErr_Clear();  \
}

#define CHECK_ERROR_THEN(descr, then) if (PyErr_Occurred()) { PRINT_ERROR(descr) { then } }
#define CHECK_ERROR(descr) CHECK_ERROR_THEN(descr, )

#endif // ERROR_MACROS