#ifndef FF_NODE_BINDINGS_HPP
#define FF_NODE_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>
#include "ff_const_bindings.hpp"

namespace py = pybind11;

class ff_node_wrapper: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;

    int svc_init() override {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            int,                /* Return type */
            ff_node_wrapper,    /* Parent class */
            svc_init,           /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

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
        // since it was returned, its ref count was decreased
        // however it is now owned by c++ side
        // so increase ref count
        retobj.inc_ref();
        //std::cout << "retobj ptr " << retobj.ptr() << std::endl;

        if (retobj.ptr() == PY_STOP.ptr()) return NULL;
        if (retobj.ptr() == PY_EOS.ptr())  return ff::FF_EOS;
        if (retobj.ptr() == PY_GO_ON.ptr()) return ff::FF_GO_ON;
        
        return (void*) ((PyObject*) retobj.ptr());
    }
    
    void svc_end() {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            void,               /* Return type */
            ff_node_wrapper,    /* Parent class */
            svc_end,            /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

    bool py_ff_send_out(py::object& task, int id = -1,
        unsigned long retry = ((unsigned long)-1),
        unsigned long ticks = (TICKS2WAIT)) {
        task.inc_ref();
        return ff_send_out((void*) ((PyObject*) task.ptr()), id, retry, ticks);
    }
};

class py_ff_node: public ff_node_wrapper {
public:
    /* Inherit the constructors */
    using ff_node_wrapper::ff_node_wrapper;

    py::object svc(py::object& obj) override {
        /* PYBIND11_OVERRIDE_PURE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE_PURE(
            py::object,         /* Return type */
            ff_node_wrapper,    /* Parent class */
            svc,                /* Name of function in C++ (must match Python name) */
            obj                 /* Argument(s) */
        );
    }
};

void ff_node_bindings(py::module_ &m) {
    py::class_<ff_node_wrapper, py_ff_node>(m, "ff_node")
        .def(py::init<>())
        .def("svc", static_cast<py::object (ff_node_wrapper::*)(py::object&)>(&ff_node_wrapper::svc), py::return_value_policy::automatic_reference)
        .def("svc_init", &ff_node_wrapper::svc_init)
        .def("svc_end", &ff_node_wrapper::svc_end)
        .def(
            "ff_send_out", 
            &ff_node_wrapper::py_ff_send_out,
            py::arg("task"),
            py::arg("id") = -1,
            py::arg("retry") = ((unsigned long)-1),
            py::arg("ticks") = (int) ff_node_wrapper::TICKS2WAIT
        );
}

#endif // FF_NODE_BINDINGS_HPP