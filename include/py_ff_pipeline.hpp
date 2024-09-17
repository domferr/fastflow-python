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
#include "py_ff_a2a.fwd.hpp"
#include "process/ff_node_process.hpp"

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
        self->use_subinterpreters = false;
        self->pipeline = NULL; 
    }
    return (PyObject*) self;
}

int py_ff_pipeline_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    py_ff_pipeline_object* m = (py_ff_pipeline_object*)self;

    PyObject* bool_arg = Py_False;
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

    PyObject *py_node = NULL;
    bool use_main_thread = false;
    if (parse_args(args, kwds, &py_node, &use_main_thread) == -1) return NULL;

    ff::ff_node* node;
    bool cleanup = true;
    bool is_a2a = PyA2A_Check(py_node);

    // we may have an a2a as stage
    if (is_a2a) {
        py_ff_a2a_object* _a2a = reinterpret_cast<py_ff_a2a_object*>(py_node);
        node = _a2a->a2a;
        cleanup = false;
    } else if (_self->use_subinterpreters) {
        node = new ff_node_subint(py_node);
    } else if (use_main_thread) {
        node = new py_ff_node(py_node);
    } else {
        node = new ff_node_process(py_node);
    }

    auto last_stage = _self->pipeline->get_laststage();
    if (last_stage) {
        // if the last stage is NOT an all-to-all then it is a simple ff_node
        // if we are adding an all-to-all, convert the simple ff_node to a ff_monode
        // if the last node is multi input then we should convert to multi output too
        if (is_a2a && !last_stage->isAll2All()) {
            if (last_stage->isMultiInput()) {
                PyErr_SetString(PyExc_RuntimeError, "Cannot transform previous stage to multi output because it has multiple inputs");
                return NULL;
            }

            // let's replace the last stage with a multi output variant
            PyObject *last_stage_py_node = _self->use_subinterpreters ? 
                (reinterpret_cast<ff_node_subint*>(last_stage))->get_python_object():
                (reinterpret_cast<ff_node_process*>(last_stage))->get_python_object();
            if (last_stage_py_node == NULL) {
                PyErr_SetString(PyExc_RuntimeError, "Unable to transform previous stage to multi output (null pointer)");
                return NULL;
            }
            ff::ff_node* new_last_stage = _self->use_subinterpreters ? 
                (ff::ff_node*) new ff_monode_subint(last_stage_py_node):
                (ff::ff_node*) new ff_monode_process(last_stage_py_node);
            auto success = _self->pipeline->change_node(last_stage, new_last_stage, cleanup, true);
            if (!success) {
                PyErr_SetString(PyExc_RuntimeError, "Unable to transform previous stage to multi output");
                return NULL;
            }
            delete last_stage;
        } else if (!is_a2a && last_stage->isAll2All()) {
            node = new ff::internal_mi_transformer(node, false);
        }
    }

    int val = _self->pipeline->add_stage(node, cleanup);
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