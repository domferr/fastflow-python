#ifndef PICKLE_HPP
#define PICKLE_HPP

#include <Python.h>
#include <iostream>

// Load pickling/un-pickling functions.
#define LOAD_PICKLE_UNPICKLE \
    PyObject* pkl_mod_name = PyUnicode_FromString("pickle");            \
    PyObject* pkl_mod = PyImport_Import(pkl_mod_name);                  \
    PyObject* pkl_dump_func = PyObject_GetAttrString(pkl_mod, "dumps"); \
    PyObject* pkl_load_func = PyObject_GetAttrString(pkl_mod, "loads");

// Clear all pickling/un-pickling functions.
#define UNLOAD_PICKLE_UNPICKLE Py_DECREF(pkl_mod_name); Py_DECREF(pkl_mod); Py_DECREF(pkl_dump_func); Py_DECREF(pkl_load_func);

// Create a PyObject* "obj" by unpickling "pickled"
#define UNPICKLE_PYOBJECT(obj, pickled) \
    PyObject* pickled_bytes = PyBytes_FromString(pickled); \
    PyObject* obj = PyObject_CallFunctionObjArgs(pkl_load_func, pickled_bytes, nullptr); \
    Py_DECREF(pickled_bytes);

// Create a std::string "str" by unpickling PyObject* "object"
#define PICKLE_PYOBJECT(str, object) \
    PyObject* protocol = PyLong_FromLong(0); \
    PyObject* decoded_bytes = PyObject_CallFunctionObjArgs(pkl_dump_func, object, protocol , nullptr); \
    std::string str = PyBytes_AsString(decoded_bytes); \
    Py_DECREF(protocol); \
    Py_DECREF(decoded_bytes);

class pickling {
public:
    pickling() {
        PyObject* pkl_mod_name = PyUnicode_FromString("pickle");
        PyObject* pkl_mod = PyImport_Import(pkl_mod_name);
        pkl_dump_func = PyObject_GetAttrString(pkl_mod, "dumps");
        pkl_load_func = PyObject_GetAttrString(pkl_mod, "loads");
        Py_DECREF(pkl_mod_name); 
        Py_DECREF(pkl_mod);
    }

    ~pickling() {
        //Py_DECREF(pkl_dump_func);
        //Py_DECREF(pkl_load_func);
    }

    std::string pickle(PyObject* object, int protocol = 5) {
        PyObject* protocol_obj = PyLong_FromLong(protocol);
        PyObject* decoded_bytes = PyObject_CallFunctionObjArgs(pkl_dump_func, object, protocol_obj , nullptr);
        std::string str = PyBytes_AsString(decoded_bytes);
        Py_DECREF(protocol_obj);
        Py_DECREF(decoded_bytes);
        return str;
    }

    PyObject* unpickle(std::string &str) {
        PyObject* pickled_bytes = PyBytes_FromString(str.c_str());
        PyObject* obj = PyObject_CallFunctionObjArgs(pkl_load_func, pickled_bytes, nullptr);
        Py_DECREF(pickled_bytes);
        return obj;
    }

private:
    PyObject* pkl_dump_func;
    PyObject* pkl_load_func;
};

#endif //PICKLE_HPP