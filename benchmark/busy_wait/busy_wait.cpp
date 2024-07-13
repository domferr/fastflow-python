#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include <cstring>
#include <ostream>
#include <sstream>
#include <sys/time.h>

static PyObject*
wait(PyObject *self, PyObject *millis_obj)
{
    auto millis = PyLong_AsLong(millis_obj);
    
    timespec ts_start;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    double start_ms = (double) (ts_start.tv_sec * 1000) + (double) (ts_start.tv_nsec / 1000000.0);

    double elapsed_time = 0;

    while (elapsed_time < (double) millis) {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double current_ms = (double) (ts.tv_sec * 1000) + (double) (ts.tv_nsec / 1000000.0);
        elapsed_time = current_ms - start_ms;
        //std::cout << millis << " " << elapsed_time << std::endl;
    }
    return Py_None;
}

static PyMethodDef busy_wait_methods[] = {
    {"wait",            (PyCFunction)wait,      METH_O,         NULL},
    {NULL,              NULL}           /* sentinel */
};

static PyModuleDef_Slot busy_wait_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

PyDoc_STRVAR(module_doc,
"busy_wait module's doc");

static struct PyModuleDef busy_wait_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "busy_wait",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = busy_wait_methods,
    .m_slots = busy_wait_slots,
    .m_clear = NULL,
    .m_free = NULL,
};

PyMODINIT_FUNC
PyInit_busy_wait(void) {
    return PyModuleDef_Init(&busy_wait_module);
}