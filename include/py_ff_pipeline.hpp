#ifndef PY_FF_PIPELINE
#define PY_FF_PIPELINE

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
#include "node_utils.hpp"

typedef struct {
    PyObject_HEAD
    bool use_subinterpreters;
    ff::ff_pipeline* pipeline;
} py_ff_pipeline_object;

PyObject *py_ff_pipeline_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    py_ff_pipeline_object *self;
    self = (py_ff_pipeline_object*) type->tp_alloc(type, 0);
    if(self != NULL){ // -> allocation successfull
        // assign initial values
        self->use_subinterpreters = true;
        self->pipeline = NULL; 
    }
    return (PyObject*) self;
}

int py_ff_pipeline_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    py_ff_pipeline_object* m = (py_ff_pipeline_object*)self;

    PyObject* bool_arg = Py_True;
    if (!PyArg_ParseTuple(args, "|O", &bool_arg)) {
        PyErr_SetString(PyExc_RuntimeError, "Error parsing tuple");
        return -1;
    } else if (bool_arg != nullptr && !PyBool_Check(bool_arg)) {
        PyErr_Format(PyExc_TypeError, "A bool is required (got type %s)",
                     Py_TYPE(bool_arg)->tp_name);
    } else {
        m->use_subinterpreters = PyObject_IsTrue(bool_arg) == 1;
    }

    m->pipeline = (ff::ff_pipeline*) PyObject_Malloc(sizeof(ff::ff_pipeline));
    if (!m->pipeline) {
        PyErr_SetString(PyExc_RuntimeError, "Memory allocation failed");
        return -1;
    }

    try {
        new (m->pipeline) ff::ff_pipeline();
    } catch (const std::exception& ex) {
        PyObject_Free(m->pipeline);
        m->pipeline = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, ex.what());
        return -1;
    } catch(...) {
        PyObject_Free(m->pipeline);
        m->pipeline = NULL;
        m->use_subinterpreters = false;
        PyErr_SetString(PyExc_RuntimeError, "Initialization failed");
        return -1;
    }

    return 0;
}

void py_ff_pipeline_dealloc(py_ff_pipeline_object *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    py_ff_pipeline_object* m = reinterpret_cast<py_ff_pipeline_object*>(self);

    if(m->pipeline) {
        m->pipeline->~ff_pipeline();
        PyObject_Free(m->pipeline);
    }

    tp->tp_free(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(py_ff_pipeline_ffTime_doc, "Return an incrmented integer");

PyObject* py_ff_pipeline_ffTime(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_pipeline_object* _self = reinterpret_cast<py_ff_pipeline_object*>(self);
    double val = _self->pipeline->ffTime();
    return PyFloat_FromDouble(val);
}

PyDoc_STRVAR(py_ff_pipeline_run_and_wait_end_doc, "Run the pipeline and wait for the end");

PyObject* py_ff_pipeline_run_and_wait_end(PyObject *self, PyObject *args)
{
    assert(self);

    py_ff_pipeline_object* _self = reinterpret_cast<py_ff_pipeline_object*>(self);
    
    // Release GIL while waiting for thread.
    int val = 0;
    Py_BEGIN_ALLOW_THREADS
    val = _self->pipeline->run_and_wait_end();
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(val);
}

PyDoc_STRVAR(py_ff_pipeline_add_stage_doc, "Add a stage to the pipeline");

PyObject* py_ff_pipeline_add_stage(PyObject *self, PyObject *args, PyObject *kwds)
{
    assert(self);

    py_ff_pipeline_object* _self = reinterpret_cast<py_ff_pipeline_object*>(self);

    ff::ff_node* node = args_to_node(args, kwds, _self->use_subinterpreters);
    if (node == NULL) return NULL;

    int val = _self->pipeline->add_stage(node, true);
    return PyLong_FromLong(val);
}

static PyMethodDef py_ff_pipeline_methods[] = {
    { "ffTime",           (PyCFunction) py_ff_pipeline_ffTime, 
        METH_NOARGS, py_ff_pipeline_ffTime_doc },
    { "run_and_wait_end", (PyCFunction) py_ff_pipeline_run_and_wait_end, 
        METH_NOARGS, py_ff_pipeline_run_and_wait_end_doc },
    { "add_stage",        (PyCFunction) py_ff_pipeline_add_stage, 
        METH_VARARGS | METH_KEYWORDS, py_ff_pipeline_add_stage_doc },
    {NULL, NULL} /* Sentinel */
};

static struct PyMemberDef py_ff_pipeline_members[] = {
    {"use_subinterpreters", T_BOOL, offsetof(py_ff_pipeline_object, use_subinterpreters)},
    {NULL} /* Sentinel */
};

/*
static PyType_Slot py_ff_pipeline_slots[] = {
    {Py_tp_new, (void*)py_ff_pipeline_new},
    {Py_tp_init, (void*)py_ff_pipeline_init},
    {Py_tp_dealloc, (void*)py_ff_pipeline_dealloc},
    {Py_tp_members, py_ff_pipeline_members},
    {Py_tp_methods, py_ff_pipeline_methods},
    {0, 0}
};

static PyType_Spec spec_py_ff_pipeline = {
    "FFPipeline",                                  // name
    sizeof(py_ff_pipeline_object) + sizeof(ff::ff_pipeline),    // basicsize
    0,                                          // itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // flags
    py_ff_pipeline_slots                               // slots
};*/

static PyTypeObject py_ff_pipeline_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastflow_module.FFPipeline",
    .tp_basicsize = sizeof(py_ff_pipeline_object) + sizeof(ff::ff_pipeline),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor) py_ff_pipeline_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("A pipeline"),
    .tp_methods = py_ff_pipeline_methods,
    .tp_members = py_ff_pipeline_members,
    .tp_init = (initproc) py_ff_pipeline_init,
    .tp_new = py_ff_pipeline_new,
};

#endif //PY_FF_PIPELINE