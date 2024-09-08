#ifndef PY_FF_NODE_PROCESS_DELEGATE
#define PY_FF_NODE_PROCESS_DELEGATE

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>

typedef struct {
    PyObject_HEAD
    std::function<PyObject*(PyObject*, int)> ff_send_out_to_callback;
} py_ff_callback_object;

PyDoc_STRVAR(py_ff_callback_ff_send_out_to_doc, "FF send out to");

PyObject* py_ff_callback_ff_send_out_to(PyObject *self, PyObject *args)
{
    assert(self);
    py_ff_callback_object* _self = reinterpret_cast<py_ff_callback_object*>(self);
    
    PyObject *pydata = nullptr;
    unsigned int index = 0;
    if (!PyArg_ParseTuple(args, "OI", &pydata, &index)) return NULL;
    
    if (pydata == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Please provide valid data object");
        return NULL;
    }

    return _self->ff_send_out_to_callback(pydata, index);
}

static PyMethodDef py_ff_callback_methods[] = {
    { "ff_send_out_to", (PyCFunction) py_ff_callback_ff_send_out_to, 
        METH_VARARGS, py_ff_callback_ff_send_out_to_doc },
    {NULL, NULL} /* Sentinel */
};

static PyTypeObject py_ff_callback_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastflow_module.FFCallback",
    .tp_basicsize = sizeof(py_ff_callback_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("A delegate"),
    .tp_methods = py_ff_callback_methods,
    .tp_new = PyType_GenericNew,
};


#endif // PY_FF_NODE_PROCESS_DELEGATE