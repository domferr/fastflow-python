#ifndef PY_FF_MONODE_PROCESS
#define PY_FF_MONODE_PROCESS

#include <Python.h>
#include <ff/ff.hpp>
#include "base_process.hpp"

#define EMPTY_TUPLE_STR "(t."

class ff_monode_process: public ff::ff_monode {
public:
    /* Inherit the constructors */
    using ff_monode::ff_monode;
    
    ff_monode_process(PyObject* node): base(node) {
        base.register_callback(this);
    }

    int run(bool arg) override {
        // We assume the main GIL is acquired
        if (this->base.run(true) < 0) return -1;
        return ff::ff_monode::run(arg);
    }

    int dryrun() override {
        // called by the previous node if this node is inside a combine
        // We assume the main GIL is acquired
        if (this->base.run(true) < 0) return -1;
        return ff::ff_monode::dryrun();
    }

    int prepare() override {
        // e.g. called if this node is inside an internal transformer
        // We assume the main GIL is acquired
        if (this->base.run(true) < 0) return -1;
        return ff::ff_monode::prepare();
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
    base_process base;
};

#endif // PY_FF_MONODE_PROCESS