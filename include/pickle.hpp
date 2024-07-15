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

    std::string pickle(PyObject* object, int protocol = 5) {
        PyObject* decoded_bytes = pickle_bytes(object, protocol);
        Py_ssize_t len;
        char *res = NULL;
        int err = PyBytes_AsStringAndSize(decoded_bytes, &res, &len);
        if (err < 0) return NULL;

        std::string str;
        str.assign(res, len);

        Py_DECREF(decoded_bytes);
        return str;
    }

    PyObject* unpickle(std::string &str) {
        PyObject* pickled_bytes = PyBytes_FromStringAndSize(str.c_str(), str.length());
        PyObject* obj = unpickle_bytes(pickled_bytes);
        Py_DECREF(pickled_bytes);
        return obj;
    }

private:
    PyObject* pkl_dump_func;
    PyObject* pkl_load_func;
};

#endif //PICKLE_HPP