#ifndef MESSAGING
#define MESSAGING

#include <Python.h>
#include "methodobject.h"
#include "modsupport.h"
#include "object.h"
#include "pyport.h"
#include "tupleobject.h"
#include "unicodeobject.h"
#include <structmember.h>
#include <ff/ff.hpp>
#include <iostream>
#include "subint/ff_monode_subint.hpp"
#include "subint/ff_node_subint.hpp"
#include "process/ff_monode_process.hpp"
#include "process/ff_node_process.hpp"
#include "python_args_utils.hpp"
#include "building_blocks_utils.hpp"
#include "py_ff_pipeline.hpp"
#include <ff/multinode.hpp>
#include "docstring_macros.hpp"

typedef struct {
    PyObject_HEAD
    Messaging channel;
    pickling *pickl;
    bool isMultiOutput;
} messaging_object;

PyObject *messaging_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    messaging_object *self;
    self = (messaging_object*) type->tp_alloc(type, 0);
    if (self != NULL) { 
        self->channel = { -1, -1 };
        self->pickl = NULL;
        self->isMultiOutput = false;
    }

    return (PyObject*) self;
}

int messaging_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    messaging_object* m = (messaging_object*)self;

    int read_fd = -1;
    int write_fd = -1;
    PyObject* pyIsMultiOutput = nullptr;
    if (!PyArg_ParseTuple(args, "iiO", &read_fd, &write_fd, &pyIsMultiOutput)) {
        PyErr_SetString(PyExc_TypeError, "Error parsing arguments");
        return -1;
    }

    if (pyIsMultiOutput == nullptr || !PyBool_Check(pyIsMultiOutput)) {
        PyErr_Format(PyExc_TypeError, "A bool is required (got type %s)",
                     Py_TYPE(pyIsMultiOutput)->tp_name);
        return -1;
    }

    m->channel = Messaging(write_fd, read_fd);
    m->pickl = new pickling();
    m->isMultiOutput = PyObject_IsTrue(pyIsMultiOutput) == 1;

    return 0;
}

void messaging_dealloc(messaging_object *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    messaging_object* m = reinterpret_cast<messaging_object*>(self);

    m->channel.closefds();
    m->channel = { -1, -1 };
    m->pickl->~pickling();
    m->pickl = NULL;

    tp->tp_free(self);
    Py_DECREF(tp);
}

doc(messaging_send_out_doc, "send_out(self, data, /)", "Send data through the communication channel");

PyObject* messaging_send_out(PyObject *self, PyObject *data)
{
    assert(self);

    messaging_object* _self = reinterpret_cast<messaging_object*>(self);
    int err = 1;
    // we may have a fastflow constant
    if (PyObject_TypeCheck(data, &py_ff_constant_type) != 0) {
        py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(data);
        err = _self->channel.send_response(_const_result->ff_const);
    } else {
        if (data == Py_None) { // map None to GO_ON
            // send GO_ON
            err = _self->channel.send_response(ff::FF_GO_ON);
        } else {
            // send serialized response
            std::string serialized;
            _self->pickl->pickle(data, serialized);
            err = _self->channel.send_response(serialized);
        }
    }

    if (err <= 0) {
        PyErr_SetString(PyExc_TypeError, "Error occurred sending data");
        return NULL;
    }

    return Py_None;
}

doc(messaging_send_out_int_doc, "send_out_int(self, number, /)", "Send a number through the communication channel. Used to send svc_init response");

PyObject* messaging_send_out_int(PyObject *self, PyObject *data)
{
    assert(self);

    messaging_object* _self = reinterpret_cast<messaging_object*>(self);
    int number = PyLong_AsLong(data);
    int err = _self->channel.send_response(number);

    if (err <= 0) {
        PyErr_SetString(PyExc_TypeError, "Error occurred sending data");
        return NULL;
    }

    return Py_None;
}

doc(messaging_get_input_doc, "get_input(self, /)", "Blocking call to get the next input from the communication channel");

PyObject* messaging_get_input(PyObject *self, PyObject *arg)
{
    assert(self);

    messaging_object* _self = reinterpret_cast<messaging_object*>(self);
    Message message;
    int err = _self->channel.recv_message(message);
    if (err <= 0) {
        PyErr_SetString(PyExc_TypeError, "Error occurred receiving the input");
        return NULL;
    }

    PyObject *dict = PyDict_New();
    bool is_eol = message.type == MESSAGE_TYPE_END_OF_LIFE;
    PyDict_SetItemString(dict, "eol", PyBool_FromLong(is_eol));
    if (!is_eol) {
        PyDict_SetItemString(dict, "fun_name", PyUnicode_FromString(message.f_name.c_str()));
        // deserialize data
        auto py_data = _self->pickl->unpickle(message.data[0]);
        PyDict_SetItemString(dict, "data", py_data);
    }

    return dict;
}

doc(messaging_ff_send_out_doc, "ff_send_out(data, index=0, /)", "Send out data to the next node. Call this function from the node's svc method to send data to its successor. You can specify an optional index if you want to send data to a specific successor node");

PyObject* messaging_ff_send_out(PyObject *self, PyObject *args) {
    assert(self);

    messaging_object* _self = reinterpret_cast<messaging_object*>(self);
    
    if (!_self->isMultiOutput) {
        PyErr_SetString(PyExc_Exception, "Operation not available. This is not a multi output node");
        return (PyObject*) NULL;
    }

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
            return NULL;
        }
    }
    
    Message response;
    if (PyObject_TypeCheck(pydata, &py_ff_constant_type) != 0) {
        // we may have a fastflow constant as data to send out to index
        py_ff_constant_object* _const_result = reinterpret_cast<py_ff_constant_object*>(pydata);
        int err = _self->channel.call_remote(response, "ff_send_out", _const_result->ff_const, index);
        if (err <= 0)  {
            PyErr_SetString(PyExc_Exception, "Error occurred sending ff_send_out request");
            return (PyObject*) NULL;
        }
    } else {
        std::string data;
        _self->pickl->pickle(pydata, data);
        int err = _self->channel.call_remote(response, "ff_send_out", data, index);
        if (PyErr_Occurred()) return (PyObject*) NULL;
        if (err <= 0)  {
            PyErr_SetString(PyExc_Exception, "Error occurred sending ff_send_out request");
            return (PyObject*) NULL;
        }
    }

    auto result = _self->channel.parse_data<bool>(response.data);
    return std::get<0>(result) ? Py_True:Py_False;
}

doc(messaging_closefds_doc, "closefds(self, /)", "Close the communication channel");

PyObject* messaging_closefds(PyObject *self, PyObject *arg)
{
    assert(self);

    messaging_object* _self = reinterpret_cast<messaging_object*>(self);
    _self->channel.closefds();

    return Py_None;
}

static PyMethodDef messaging_methods[] = {
    { "send_out", (PyCFunction) messaging_send_out, 
        METH_O, messaging_send_out_doc },
    { "send_out_int", (PyCFunction) messaging_send_out_int, 
        METH_O, messaging_send_out_int_doc },
    { "get_input", (PyCFunction) messaging_get_input, 
        METH_NOARGS, messaging_get_input_doc },
    { "ff_send_out", (PyCFunction) messaging_ff_send_out, 
        METH_VARARGS, messaging_ff_send_out_doc },
    { "closefds", (PyCFunction) messaging_closefds, 
        METH_NOARGS, messaging_closefds_doc },
    {NULL, NULL} /* Sentinel */
};

static struct PyMemberDef messaging_members[] = {
    {NULL} /* Sentinel */
};

PyTypeObject messaging_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastflow.Messaging",
    .tp_basicsize = sizeof(messaging_object) + sizeof(Messaging) + sizeof(pickling*),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor) messaging_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Communication channel for FastFlow's multiprocessing implementation"),
    .tp_methods = messaging_methods,
    .tp_members = messaging_members,
    .tp_init = (initproc) messaging_init,
    .tp_new = messaging_new,
};

#endif //MESSAGING