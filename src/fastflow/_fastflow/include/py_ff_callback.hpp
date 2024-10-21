#ifndef PY_FF_NODE_PROCESS_DELEGATE
#define PY_FF_NODE_PROCESS_DELEGATE

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>

typedef struct {
    PyObject_HEAD
    std::function<PyObject*(PyObject*, int)> ff_send_out_callback;
} py_ff_callback_object;

PyDoc_STRVAR(py_ff_callback_ff_send_out_doc, "FF send out to");

PyObject* py_ff_callback_ff_send_out(PyObject *self, PyObject *args)
{
    assert(self);
    py_ff_callback_object* _self = reinterpret_cast<py_ff_callback_object*>(self);
    
    PyObject *pydata = nullptr;
    PyObject *pyindex = nullptr;
    // doc: http://web.mit.edu/people/amliu/vrut/python/ext/parseTuple.html
    if (!PyArg_ParseTuple(args, "O|O", &pydata, &pyindex)) return NULL;
    
    if (pydata == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Please provide valid data object");
        return NULL;
    }

    int index = -1;
    if (pyindex != nullptr) {
        if (PyLong_Check(pyindex) == 0) {
            PyErr_Format(PyExc_TypeError, "Please provide the index as an integer object (got type %s)",
                        Py_TYPE(pyindex)->tp_name);
            return NULL;
        }

        index = PyLong_AsLong(pyindex);

        if (index < 0) {
            PyErr_SetString(PyExc_Exception, "Index cannot be negative");
            return (PyObject*) NULL;
        }
    }

    return _self->ff_send_out_callback(pydata, index);
}

static PyMethodDef py_ff_callback_methods[] = {
    { "ff_send_out", (PyCFunction) py_ff_callback_ff_send_out, 
        METH_VARARGS, py_ff_callback_ff_send_out_doc },
    {NULL, NULL} /* Sentinel */
};

static PyTypeObject py_ff_callback_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastflow.FFCallback",
    .tp_basicsize = sizeof(py_ff_callback_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("A delegate"),
    .tp_methods = py_ff_callback_methods,
    .tp_new = PyType_GenericNew,
};


#endif // PY_FF_NODE_PROCESS_DELEGATE