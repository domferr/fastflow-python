#ifndef PY_FF_CONSTANTS_DELEGATE
#define PY_FF_CONSTANTS_DELEGATE

#include <Python.h>

typedef struct {
    PyObject_HEAD
    void* ff_const;
} py_ff_constant_object;

static PyTypeObject py_ff_constant_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastflow.FFConstant",
    .tp_basicsize = sizeof(py_ff_constant_object) + sizeof(void*),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("A fastflow's constant"),
    .tp_new = PyType_GenericNew,
};

#define GO_ON_CONSTANT_NAME "GO_ON"
#define EOS_CONSTANT_NAME "EOS"

PyObject* build_py_ff_constant(void *val) {
    py_ff_constant_object* constant = (py_ff_constant_object*) PyObject_CallObject(
        (PyObject *) &py_ff_constant_type, NULL
    );
    constant->ff_const = val;
    return (PyObject*) constant;
}

#endif // PY_FF_CONSTANTS_DELEGATE