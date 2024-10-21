#ifndef PY_FF_NODE_SUBINT
#define PY_FF_NODE_SUBINT

#include <Python.h>
#include <ff/ff.hpp>
#include "base_subint.hpp"

#define EMPTY_TUPLE_STR "(t."

class ff_node_subint: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;
    
    ff_node_subint(PyObject* node, bool is_multi_output = true): base(node, is_multi_output) {

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
    base_subint base;
};

#endif // PY_FF_NODE_SUBINT