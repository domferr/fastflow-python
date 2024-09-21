#ifndef PICKLE_HPP
#define PICKLE_HPP

#include <Python.h>
#include <iostream>

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

    PyObject* pickle_bytes(PyObject* object, int protocol = 5) {
        PyObject* protocol_obj = PyLong_FromLong(protocol);
        PyObject* decoded_bytes = PyObject_CallFunctionObjArgs(pkl_dump_func, object, protocol_obj, nullptr);
        Py_DECREF(protocol_obj);
        return decoded_bytes;
    }

    PyObject* unpickle_bytes(PyObject* pickled_bytes) {
        return PyObject_CallFunctionObjArgs(pkl_load_func, pickled_bytes, nullptr);
    }

    int pickle(PyObject* object, char **str, Py_ssize_t *len, int protocol = 5) {
        PyObject* decoded_bytes = pickle_bytes(object, protocol);
        *str = NULL;
        return PyBytes_AsStringAndSize(decoded_bytes, str, len);
    }

    PyObject* unpickle(char* str, long size) {
        PyObject* pickled_bytes = PyBytes_FromStringAndSize(str, size);
        PyObject* obj = unpickle_bytes(pickled_bytes);
        Py_DECREF(pickled_bytes);
        return obj;
    }

private:
    PyObject* pkl_dump_func;
    PyObject* pkl_load_func;
};

#endif //PICKLE_HPP