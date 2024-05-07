#ifndef FF_MONODE_BINDINGS_HPP
#define FF_MONODE_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>
#include "ff_const_bindings.hpp"

namespace py = pybind11;

// todo: it would be better to reuse the bindings of ff_node
class ff_monode_wrapper: public ff::ff_monode {
public:
    /* Inherit the constructors */
    using ff_monode::ff_monode;

    virtual py::object svc(py::object& obj) = 0;

    void * svc(void *arg) override {
        // Acquire the GIL while in this scope.
        py::gil_scoped_acquire gil;
        
        // if the argument is NULL, pass an empty tuple to python function
        py::object obj = py::make_tuple();
        if (arg != NULL) {
            // move operation. it keeps the same ref count instead of increasing
            obj = py::reinterpret_steal<py::object>((PyObject*) arg);
           // std::cout << "build object from argument. (ref count = " << obj.ref_count() << ")" << std::endl;
        }
        auto retobj = svc(obj);

        if (retobj.is(PY_STOP)) return NULL;
        else if (retobj.is(PY_EOS)) return ff::FF_EOS;
        else if (retobj.is(PY_GO_ON)) return ff::FF_GO_ON;
        else {
            // since it was returned, its ref count was decreased
            // however it is now owned by c++ side
            // so increase ref count
            retobj.inc_ref();
        }
        
        return (void*) ((PyObject*) retobj.ptr());
    }

    bool py_ff_send_out(py::object& obj, int id = -1,
        unsigned long retry = ((unsigned long)-1),
        unsigned long ticks = (TICKS2WAIT)
    ) {
        void * task = (void*) ((PyObject*) obj.ptr());

        if (obj.is(PY_STOP)) task = NULL;
        else if (obj.is(PY_EOS)) task = ff::FF_EOS;
        else if (obj.is(PY_GO_ON)) task = ff::FF_GO_ON;

        return ff_send_out(task, id, retry, ticks);
    }

    bool py_ff_send_out_to(py::object& obj, int id,
        unsigned long retry = ((unsigned long)-1),
        unsigned long ticks = (TICKS2WAIT)
    ) {
        void * task = (void*) ((PyObject*) obj.ptr());

        if (obj.is(PY_STOP)) task = NULL;
        else if (obj.is(PY_EOS)) task = ff::FF_EOS;
        else if (obj.is(PY_GO_ON)) task = ff::FF_GO_ON;

        return ff_send_out_to(task, id, retry, ticks);
    }
};

class py_ff_monode: public ff_monode_wrapper {
public:
    /* Inherit the constructors */
    using ff_monode_wrapper::ff_monode_wrapper;

    int svc_init() override {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            int,                /* Return type */
            ff_monode_wrapper,  /* Parent class */
            svc_init,           /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

    py::object svc(py::object& obj) override {
        /* PYBIND11_OVERRIDE_PURE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE_PURE(
            py::object,         /* Return type */
            ff_monode_wrapper,  /* Parent class */
            svc,                /* Name of function in C++ (must match Python name) */
            obj                 /* Argument(s) */
        );
    }
    
    void svc_end() {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            void,               /* Return type */
            ff_monode_wrapper,  /* Parent class */
            svc_end,            /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

    void eosnotify(ssize_t id = -1) override {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            void,               /* Return type */
            ff_monode_wrapper,  /* Parent class */
            eosnotify,          /* Name of function in C++ (must match Python name) */
            id                  /* Argument(s) */
        );
    }
};

void ff_monode_bindings(py::module_ &m) {
    py::class_<ff_monode_wrapper, py_ff_monode>(m, "ff_monode")
        .def(py::init<>())
        .def("svc", static_cast<py::object (ff_monode_wrapper::*)(py::object&)>(&ff_monode_wrapper::svc), py::return_value_policy::automatic_reference)
        .def("svc_init", &ff_monode_wrapper::svc_init)
        .def("svc_end", &ff_monode_wrapper::svc_end)
        .def(
            "ff_send_out", 
            &ff_monode_wrapper::py_ff_send_out,
            py::keep_alive<1, 2>(),
            py::arg("task"),
            py::arg("id") = -1,
            py::arg("retry") = ((unsigned long)-1),
            py::arg("ticks") = (int) ff_monode_wrapper::TICKS2WAIT
        )
        .def(
            "ff_send_out_to", 
            &ff_monode_wrapper::py_ff_send_out_to,
            py::keep_alive<1, 2>(),
            py::arg("task"),
            py::arg("id"),
            py::arg("retry") = ((unsigned long)-1),
            py::arg("ticks") = (int) ff_monode_wrapper::TICKS2WAIT
        )
        .def("eosnotify", &ff_monode_wrapper::eosnotify)
        .def("get_channel_id", &ff_monode_wrapper::get_channel_id);
}

#endif // FF_MIOODE_BINDINGS_HPP