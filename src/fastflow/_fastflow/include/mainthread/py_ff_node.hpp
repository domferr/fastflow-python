#ifndef PY_FF_NODE
#define PY_FF_NODE

#include <Python.h>
#include <ff/ff.hpp>
#include "base_mainthread.hpp"

class py_ff_node: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;
    
    py_ff_node(PyObject* node): base(node) {

    }
    
    int svc_init() override {
        return base.svc_init();
    }

    void * svc(void *arg) override {
        return base.svc(arg);
    }

    void svc_end() override {
        base.svc_end();
    }

    PyObject* get_python_object() {
        return base.get_python_object();
    }

private:
    base_mainthread base;
};

#endif // PY_FF_NODE