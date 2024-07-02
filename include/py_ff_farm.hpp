#ifndef PY_FF_FARM
#define PY_FF_FARM

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

typedef struct {
    PyObject_HEAD
    bool use_subinterpreters;
    ff::ff_farm* farm;
} py_ff_farm_object;

PyObject *py_ff_farm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    std::cout << "py_ff_farm_new() called!" << std::endl;

    py_ff_farm_object *self;
    self = (py_ff_farm_object*) type->tp_alloc(type, 0);
    if(self != NULL){ // -> allocation successfull
        // assign initial values
        self->use_subinterpreters = true;
        self->farm = NULL; 
    }
    return (PyObject*) self;
}

int py_ff_farm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    std::cout << "py_ff_farm_init() called!" << std::endl;
    py_ff_farm_object* m = (py_ff_farm_object*)self;

    PyObject* bool_arg = Py_True;
    if (!PyArg_ParseTuple(args, "|O", &bool_arg)) {
        std::cerr << "error parsing tuple" << std::endl;
    } else if (bool_arg != nullptr && !PyBool_Check(bool_arg)) {
        PyErr_Format(PyExc_TypeError, "A bool is required (got type %s)",
                     Py_TYPE(bool_arg)->tp_name);
    } else {
        m->use_subinterpreters = PyObject_IsTrue(bool_arg) == 1;
    }

    m->farm = (ff::ff_farm*) PyObject_Malloc(sizeof(ff::ff_farm));
    if (!m->farm) {
        PyErr_SetString(PyExc_RuntimeError, "Memory allocation failed");
        return -1;
    }

    try {
        new (m->farm) ff::ff_farm();
    } catch (const std::exception& ex) {
        PyObject_Free(m->farm);
        m->farm = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, ex.what());
        return -1;
    } catch(...) {
        PyObject_Free(m->farm);
        m->farm = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, "Initialization failed");
        return -1;
    }

    return 0;
}

void py_ff_farm_dealloc(py_ff_farm_object *self)
{
    std::cout << "py_ff_farm_dealloc() called!" << std::endl;
    PyTypeObject *tp = Py_TYPE(self);

    py_ff_farm_object* m = reinterpret_cast<py_ff_farm_object*>(self);

    if(m->farm) {
        m->farm->~ff_farm();
        PyObject_Free(m->farm);
    }

    tp->tp_free(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(py_ff_farm_ffTime_doc, "Return an incrmented integer");

PyObject* py_ff_farm_ffTime(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    double val = _self->farm->ffTime();
    return PyFloat_FromDouble(val);
}

PyDoc_STRVAR(py_ff_farm_run_and_wait_end_doc, "Run the pipeline and wait for the end");

PyObject* py_ff_farm_run_and_wait_end(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    // Release GIL while waiting for thread.
    int val = 0;
    Py_BEGIN_ALLOW_THREADS
    val = _self->farm->run_and_wait_end();
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_emitter_doc, "Add emitter to the farm");

PyObject* py_ff_farm_add_emitter(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node;
    if (_self->use_subinterpreters) node = new py_ff_node_subint(arg);
    else node = new py_ff_node(arg);

    int val = _self->farm->add_emitter(node);
    _self->farm->cleanup_emitter(true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_emitter_process_doc, "Add emitter to the farm that runs in another process");

PyObject* py_ff_farm_add_emitter_process(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node = new py_ff_node_process(arg);

    int val = _self->farm->add_emitter(node);
    _self->farm->cleanup_emitter(true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_workers_doc, "Add workers to the farm");

PyObject* py_ff_farm_add_workers(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    std::vector<ff::ff_node*> workers;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        ff::ff_node* node;
        if (_self->use_subinterpreters) node = new py_ff_node_subint(item);
        else node = new py_ff_node(item);
        workers.push_back(node);
    }
    Py_DECREF(iterator);

    int val = _self->farm->add_workers(workers);
    _self->farm->cleanup_workers(true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_workers_process_doc, "Add workers to the farm that run in another process each");

PyObject* py_ff_farm_add_workers_process(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);

    std::vector<ff::ff_node*> workers;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(arg);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        workers.push_back(new py_ff_node_process(item));
    }
    Py_DECREF(iterator);

    int val = _self->farm->add_workers(workers);
    _self->farm->cleanup_workers(true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_collector_doc, "Add collector to the farm");

PyObject* py_ff_farm_add_collector(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node;
    if (_self->use_subinterpreters) node = new py_ff_node_subint(arg);
    else node = new py_ff_node(arg);

    int val = _self->farm->add_collector(node, true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_collector_process_doc, "Add collector to the farm that runs in another process");

PyObject* py_ff_farm_add_collector_process(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node = new py_ff_node_process(arg);

    int val = _self->farm->add_collector(node, true);
    return PyLong_FromLong(val);
}

static PyMethodDef py_ff_farm_methods[] = {
    { "ffTime",           (PyCFunction) py_ff_farm_ffTime, 
        METH_NOARGS, py_ff_farm_ffTime_doc },
    { "run_and_wait_end", (PyCFunction) py_ff_farm_run_and_wait_end, 
        METH_NOARGS, py_ff_farm_run_and_wait_end_doc },
    { "add_workers",      (PyCFunction) py_ff_farm_add_workers, 
        METH_O, py_ff_farm_add_workers_doc },
    { "add_workers_process",   (PyCFunction) py_ff_farm_add_workers_process, 
        METH_O, py_ff_farm_add_workers_process_doc },
    { "add_emitter",      (PyCFunction) py_ff_farm_add_emitter, 
        METH_O, py_ff_farm_add_emitter_doc },
    { "add_emitter_process",   (PyCFunction) py_ff_farm_add_emitter_process, 
        METH_O, py_ff_farm_add_emitter_process_doc },
    { "add_collector",    (PyCFunction) py_ff_farm_add_collector, 
        METH_O, py_ff_farm_add_collector_doc },
    { "add_collector_process", (PyCFunction) py_ff_farm_add_collector_process, 
        METH_O, py_ff_farm_add_collector_process_doc },
    {NULL, NULL} /* Sentinel */
};

static struct PyMemberDef py_ff_farm_members[] = {
    {"use_subinterpreters", T_BOOL, offsetof(py_ff_farm_object, use_subinterpreters)},
    {NULL} /* Sentinel */
};

static PyType_Slot py_ff_farm_slots[] = {
    {Py_tp_new, (void*)py_ff_farm_new},
    {Py_tp_init, (void*)py_ff_farm_init},
    {Py_tp_dealloc, (void*)py_ff_farm_dealloc},
    {Py_tp_members, py_ff_farm_members},
    {Py_tp_methods, py_ff_farm_methods},
    {0, 0}
};

static PyType_Spec spec_py_ff_farm = {
    "FFFarm",                                  // name
    sizeof(py_ff_farm_object) + sizeof(ff::ff_pipeline),    // basicsize
    0,                                          // itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // flags
    py_ff_farm_slots                               // slots
};

#endif //PY_FF_FARM