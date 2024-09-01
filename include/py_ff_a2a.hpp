#ifndef PY_FF_ALLTOALL
#define PY_FF_ALLTOALL

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
#include "py_ff_node.hpp"
#include "py_ff_node_subint.hpp"
#include "py_ff_node_process.hpp"
#include <ff/multinode.hpp>

typedef struct {
    PyObject_HEAD
    bool use_subinterpreters;
    ff::ff_a2a* a2a;
} py_ff_a2a_object;

PyObject *py_ff_a2a_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    py_ff_a2a_object *self;
    self = (py_ff_a2a_object*) type->tp_alloc(type, 0);
    if(self != NULL){ // -> allocation successfull
        // assign initial values
        self->use_subinterpreters = true;
        self->a2a = NULL; 
    }
    return (PyObject*) self;
}

int py_ff_a2a_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    py_ff_a2a_object* m = (py_ff_a2a_object*)self;

    PyObject* bool_arg = Py_True;
    if (!PyArg_ParseTuple(args, "|O", &bool_arg)) {
        std::cerr << "error parsing tuple" << std::endl;
    } else if (bool_arg != nullptr && !PyBool_Check(bool_arg)) {
        PyErr_Format(PyExc_TypeError, "A bool is required (got type %s)",
                     Py_TYPE(bool_arg)->tp_name);
    } else {
        m->use_subinterpreters = PyObject_IsTrue(bool_arg) == 1;
    }

    m->a2a = (ff::ff_a2a*) PyObject_Malloc(sizeof(ff::ff_a2a));
    if (!m->a2a) {
        PyErr_SetString(PyExc_RuntimeError, "Memory allocation failed");
        return -1;
    }

    try {
        new (m->a2a) ff::ff_a2a();
    } catch (const std::exception& ex) {
        PyObject_Free(m->a2a);
        m->a2a = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, ex.what());
        return -1;
    } catch(...) {
        PyObject_Free(m->a2a);
        m->a2a = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, "Initialization failed");
        return -1;
    }

    return 0;
}

void py_ff_a2a_dealloc(py_ff_a2a_object *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    py_ff_a2a_object* m = reinterpret_cast<py_ff_a2a_object*>(self);

    if(m->a2a) {
        m->a2a->~ff_a2a();
        PyObject_Free(m->a2a);
    }

    tp->tp_free(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(py_ff_a2a_ffTime_doc, "Return the time elapsed");

PyObject* py_ff_a2a_ffTime(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);
    double val = _self->a2a->ffTime();
    return PyFloat_FromDouble(val);
}

PyDoc_STRVAR(py_ff_a2a_run_and_wait_end_doc, "Run the all to all and wait for the end");

PyObject* py_ff_a2a_run_and_wait_end(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);
    
    // Release GIL while waiting for thread.
    int val = 0;
    Py_BEGIN_ALLOW_THREADS
    val = _self->a2a->run_and_wait_end();
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_a2a_add_firstset_doc, "Add first set to the a2a");

PyObject* py_ff_a2a_add_firstset(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);

    std::vector<ff::ff_node*> set;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        ff::ff_node* node;
        // we may have a pipeline as item
        if (PyObject_TypeCheck(item, &py_ff_pipeline_type) != 0) {
            py_ff_pipeline_object* _pipe = reinterpret_cast<py_ff_pipeline_object*>(item);
            if (!_pipe->pipeline->isMultiOutput()) {
                ff::ff_node* last_stage = _pipe->pipeline->get_laststage();
                ff::ff_node* new_node = new ff::internal_mo_transformer(last_stage, false);
                _pipe->pipeline->change_node(last_stage, new_node, true, true);
            }
            node = _pipe->pipeline;
        } else if (_self->use_subinterpreters) {
            node = new py_ff_node_subint(item);
        } else {
            node = new py_ff_node(item);
        }
        set.push_back(node);
    }
    Py_DECREF(iterator);

    int ondemand = 0;
    int val = _self->a2a->add_firstset(set, ondemand, false);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_a2a_add_firstset_process_doc, "Add first set to the a2a that runs in another process");

PyObject* py_ff_a2a_add_firstset_process(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);

    std::vector<ff::ff_node*> set;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        ff::ff_node* node;
        // we may have a pipeline as item
        if (PyObject_TypeCheck(item, &py_ff_pipeline_type) != 0) {
            py_ff_pipeline_object* _pipe = reinterpret_cast<py_ff_pipeline_object*>(item);
            if (!_pipe->pipeline->isMultiOutput()) {
                auto last_stage = _pipe->pipeline->get_laststage();
                auto new_node = new ff::internal_mo_transformer(last_stage, false);
                _pipe->pipeline->change_node(last_stage, new_node, true, true);
            }
            node = _pipe->pipeline;
        } else {
            node = new py_ff_node_process(item);
        }
        set.push_back(node);
    }
    Py_DECREF(iterator);

    int ondemand = 0;
    int val = _self->a2a->add_firstset(set, ondemand, false);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_a2a_add_secondset_doc, "Add second set to the a2a");

PyObject* py_ff_a2a_add_secondset(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);

    std::vector<ff::ff_node*> set;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        ff::ff_node* node;
        if (_self->use_subinterpreters) node = new py_ff_node_subint(item);
        else node = new py_ff_node(item);
        set.push_back(node);
    }
    Py_DECREF(iterator);

    int val = _self->a2a->add_secondset(set, true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_a2a_add_secondset_process_doc, "Add second set to the a2a that runs in another process");

PyObject* py_ff_a2a_add_secondset_process(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);

    std::vector<ff::ff_node*> set;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        set.push_back(new py_ff_node_process(item));
    }
    Py_DECREF(iterator);

    int val = _self->a2a->add_secondset(set, true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_a2a_blocking_mode_doc, "Set all to all's blocking mode");

PyObject* py_ff_a2a_blocking_mode(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);

    PyObject* bool_arg = PyBool_Check(arg) == 1 ? arg:Py_False;
    bool enable = PyObject_IsTrue(bool_arg) == 1;
    _self->a2a->blocking_mode(enable);

    return Py_None;
}

PyDoc_STRVAR(py_ff_a2a_no_mapping_doc, "Disable fastflow's mapping for this a2a");

PyObject* py_ff_a2a_no_mapping(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_a2a_object* _self = reinterpret_cast<py_ff_a2a_object*>(self);
    _self->a2a->no_mapping();

    return Py_None;
}

static PyMethodDef py_ff_a2a_methods[] = {
    { "ffTime",           (PyCFunction) py_ff_a2a_ffTime, 
        METH_NOARGS, py_ff_a2a_ffTime_doc },
    { "run_and_wait_end", (PyCFunction) py_ff_a2a_run_and_wait_end, 
        METH_NOARGS, py_ff_a2a_run_and_wait_end_doc },
    { "add_firstset",      (PyCFunction) py_ff_a2a_add_firstset, 
        METH_O, py_ff_a2a_add_firstset_doc },
    { "add_firstset_process",   (PyCFunction) py_ff_a2a_add_firstset_process, 
        METH_O, py_ff_a2a_add_firstset_process_doc },
    { "add_secondset",      (PyCFunction) py_ff_a2a_add_secondset, 
        METH_O, py_ff_a2a_add_firstset_doc },
    { "add_secondset_process",   (PyCFunction) py_ff_a2a_add_secondset_process, 
        METH_O, py_ff_a2a_add_secondset_process_doc },
    { "blocking_mode", (PyCFunction) py_ff_a2a_blocking_mode, 
        METH_O, py_ff_a2a_blocking_mode_doc },
    { "no_mapping", (PyCFunction) py_ff_a2a_no_mapping, 
        METH_NOARGS, py_ff_a2a_no_mapping_doc },
    {NULL, NULL} /* Sentinel */
};

static struct PyMemberDef py_ff_a2a_members[] = {
    {"use_subinterpreters", T_BOOL, offsetof(py_ff_a2a_object, use_subinterpreters)},
    {NULL} /* Sentinel */
};

static PyType_Slot py_ff_a2a_slots[] = {
    {Py_tp_new, (void*)py_ff_a2a_new},
    {Py_tp_init, (void*)py_ff_a2a_init},
    {Py_tp_dealloc, (void*)py_ff_a2a_dealloc},
    {Py_tp_members, py_ff_a2a_members},
    {Py_tp_methods, py_ff_a2a_methods},
    {0, 0}
};

static PyType_Spec spec_py_ff_a2a = {
    "FFAllToAll",                                  // name
    sizeof(py_ff_a2a_object) + sizeof(ff::ff_pipeline),    // basicsize
    0,                                          // itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // flags
    py_ff_a2a_slots                               // slots
};

#endif //PY_FF_ALLTOALL