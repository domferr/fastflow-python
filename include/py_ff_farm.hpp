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
#include "subint/ff_node_subint.hpp"
#include "process/ff_node_process.hpp"
#include "python_args_utils.hpp"
#include "building_blocks_utils.hpp"

typedef struct {
    PyObject_HEAD
    bool use_subinterpreters;
    ff::ff_farm* farm;
    ff::ff_pipeline* accelerator;
} py_ff_farm_object;

PyObject *py_ff_farm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    py_ff_farm_object *self;
    self = (py_ff_farm_object*) type->tp_alloc(type, 0);
    if(self != NULL){ // -> allocation successfull
        // assign initial values
        self->use_subinterpreters = false;
        self->farm = NULL;
        self->accelerator = NULL;
    }
    return (PyObject*) self;
}

int py_ff_farm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    py_ff_farm_object* m = (py_ff_farm_object*)self;

    PyObject* bool_arg = Py_False;
    if (!PyArg_ParseTuple(args, "|O", &bool_arg)) {
        PyErr_SetString(PyExc_TypeError, "Error parsing arguments");
        return -1;
    } else if (bool_arg != nullptr && !PyBool_Check(bool_arg)) {
        PyErr_Format(PyExc_TypeError, "A bool is required (got type %s)",
                     Py_TYPE(bool_arg)->tp_name);
    } else {
        m->use_subinterpreters = PyObject_IsTrue(bool_arg) == 1;
        // TODO: if we have problems with the following, then we could to PySys_GetObject("version_info");
#if PY_MINOR_VERSION < 12
        if (m->use_subinterpreters) {
            PyErr_SetString(PyExc_TypeError, "Subinterpreters are supported from Python 3.12, but you are using and older version");
            return -1;
        }
#endif
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
        m->accelerator = NULL;
        PyErr_SetString(PyExc_RuntimeError, ex.what());
        return -1;
    } catch(...) {
        PyObject_Free(m->farm);
        m->farm = NULL;
        m->use_subinterpreters = false;
        m->accelerator = NULL;
        PyErr_SetString(PyExc_RuntimeError, "Initialization failed");
        return -1;
    }

    return 0;
}

void py_ff_farm_dealloc(py_ff_farm_object *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    py_ff_farm_object* m = reinterpret_cast<py_ff_farm_object*>(self);

    if(m->farm) {
        m->farm->~ff_farm();
        PyObject_Free(m->farm);
        if (m->accelerator) m->accelerator->~ff_pipeline();
    }

    tp->tp_free(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(py_ff_farm_ffTime_doc, "Return the time elapsed");

PyObject* py_ff_farm_ffTime(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    double val = _self->farm->ffTime();
    return PyFloat_FromDouble(val);
}

PyDoc_STRVAR(py_ff_farm_run_and_wait_end_doc, "Run the farm and wait for the end");

PyObject* py_ff_farm_run_and_wait_end(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    return run_and_wait_end(_self->farm, _self->use_subinterpreters);
}

PyDoc_STRVAR(py_ff_farm_run_doc, "Run the farm asynchronously");

PyObject* py_ff_farm_run(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    run_accelerator(&_self->accelerator, _self->farm, _self->use_subinterpreters);
    return Py_None;
}

PyDoc_STRVAR(py_ff_farm_wait_doc, "Wait for the farm to complete all its tasks");

PyObject* py_ff_farm_wait(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    return wait(_self->farm, _self->use_subinterpreters);
}

PyDoc_STRVAR(py_ff_farm_submit_doc, "Submit data to first stage of the farm");

PyObject* py_ff_farm_submit(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    return submit(_self->accelerator, arg);
}

PyDoc_STRVAR(py_ff_farm_collect_next_doc, "Collect next output data");

PyObject* py_ff_farm_collect_next(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    return collect_next(_self->accelerator);
}

PyDoc_STRVAR(py_ff_farm_add_emitter_doc, "Add emitter to the farm");

PyObject* py_ff_farm_add_emitter(PyObject *self, PyObject *args, PyObject *kwds)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node = args_to_node(args, kwds, _self->use_subinterpreters, true);
    if (node == NULL) return NULL;

    int val = _self->farm->add_emitter(node);

    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_workers_doc, "Add workers to the farm");

PyObject* py_ff_farm_add_workers(PyObject *self, PyObject *args, PyObject *kwds)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);

    PyObject *py_nodes = NULL;
    bool use_main_thread = false;
    if (parse_args(args, kwds, &py_nodes, &use_main_thread) == -1) return NULL;

    std::vector<ff::ff_node*> workers;
    // Iterate over all of the arguments.
    PyObject* iterator = PyObject_GetIter(py_nodes);
    PyObject* item;
    while ((item = PyIter_Next(iterator))) {
        ff::ff_node* node;
        if (_self->use_subinterpreters) {
            node = new ff_node_subint(item);
        } else if (use_main_thread) {
            node = new py_ff_node(item);
        } else {
            node = new ff_node_process(item);
        }
        workers.push_back(node);
    }
    Py_DECREF(iterator);

    int val = _self->farm->add_workers(workers);
    _self->farm->cleanup_workers(true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_add_collector_doc, "Add collector to the farm");

PyObject* py_ff_farm_add_collector(PyObject *self, PyObject *args, PyObject *kwds)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    
    ff::ff_node* node = args_to_node(args, kwds, _self->use_subinterpreters);
    if (node == NULL) return NULL;

    int val = _self->farm->add_collector(node, true);
    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_farm_blocking_mode_doc, "Set farm's blocking mode");

PyObject* py_ff_farm_blocking_mode(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);

    PyObject* bool_arg = PyBool_Check(arg) == 1 ? arg:Py_False;
    bool enable = PyObject_IsTrue(bool_arg) == 1;
    _self->farm->blocking_mode(enable);

    return Py_None;
}

PyDoc_STRVAR(py_ff_farm_no_mapping_doc, "Disable fastflow's mapping for this farm");

PyObject* py_ff_farm_no_mapping(PyObject *self, PyObject *arg)
{
    assert(self);

    py_ff_farm_object* _self = reinterpret_cast<py_ff_farm_object*>(self);
    _self->farm->no_mapping();

    return Py_None;
}

static PyMethodDef py_ff_farm_methods[] = {
    { "ffTime",           (PyCFunction) py_ff_farm_ffTime, 
        METH_NOARGS, py_ff_farm_ffTime_doc },
    { "run_and_wait_end", (PyCFunction) py_ff_farm_run_and_wait_end, 
        METH_NOARGS, py_ff_farm_run_and_wait_end_doc },
    { "run", (PyCFunction) py_ff_farm_run, 
        METH_NOARGS, py_ff_farm_run_doc },
    { "wait", (PyCFunction) py_ff_farm_wait, 
        METH_NOARGS, py_ff_farm_wait_doc },
    { "submit", (PyCFunction) py_ff_farm_submit, 
        METH_O, py_ff_farm_submit_doc },
    { "collect_next", (PyCFunction) py_ff_farm_collect_next, 
        METH_NOARGS, py_ff_farm_collect_next_doc },
    { "add_workers",      (PyCFunction) py_ff_farm_add_workers, 
        METH_VARARGS | METH_KEYWORDS, py_ff_farm_add_workers_doc },
    { "add_emitter",      (PyCFunction) py_ff_farm_add_emitter, 
        METH_VARARGS | METH_KEYWORDS, py_ff_farm_add_emitter_doc },
    { "add_collector",    (PyCFunction) py_ff_farm_add_collector, 
        METH_VARARGS | METH_KEYWORDS, py_ff_farm_add_collector_doc },
    { "blocking_mode", (PyCFunction) py_ff_farm_blocking_mode, 
        METH_O, py_ff_farm_blocking_mode_doc },
    { "no_mapping", (PyCFunction) py_ff_farm_no_mapping, 
        METH_NOARGS, py_ff_farm_no_mapping_doc },
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
    sizeof(py_ff_farm_object) + sizeof(ff::ff_farm),    // basicsize
    0,                                          // itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // flags
    py_ff_farm_slots                               // slots
};

#endif //PY_FF_FARM