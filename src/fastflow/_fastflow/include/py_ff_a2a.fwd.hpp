#ifndef PY_FF_ALLTOALL_FWD
#define PY_FF_ALLTOALL_FWD

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
#include "py_ff_pipeline.hpp"
#include <ff/multinode.hpp>

typedef struct {
    PyObject_HEAD
    bool use_subinterpreters;
    ff::ff_a2a* a2a;
    ff::ff_pipeline* accelerator;
} py_ff_a2a_object;

extern PyTypeObject py_ff_a2a_type;

#define PyA2A_Check(obj) PyObject_TypeCheck(obj, &py_ff_a2a_type) != 0

#endif //PY_FF_ALLTOALL_FWD