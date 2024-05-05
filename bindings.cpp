#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/functional.h>
#include <ff/ff.hpp>

namespace py = pybind11;

auto PY_STOP  = py::cast(NULL);
auto PY_GO_ON = py::cast(ff::FF_GO_ON);
auto PY_EOS   = py::cast(ff::FF_EOS);

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

    virtual py::object svc(py::object obj) = 0;

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

    bool py_ff_send_out(py::object task, int id = -1,
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

    py::object svc(py::object obj) override {
        /* PYBIND11_OVERRIDE_PURE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE_PURE(
            py::object,         /* Return type */
            ff_node_wrapper,    /* Parent class */
            svc,                /* Name of function in C++ (must match Python name) */
            obj                 /* Argument(s) */
        );
    }
};

// todo: it would be better to reuse the bindings of ff_node
class ff_minode_wrapper: public ff::ff_minode {
public:
    /* Inherit the constructors */
    using ff_minode::ff_minode;

    int svc_init() override {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            int,                /* Return type */
            ff_minode_wrapper,  /* Parent class */
            svc_init,           /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

    virtual py::object svc(py::object obj) = 0;

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
            ff_minode_wrapper,  /* Parent class */
            svc_end,            /* Name of function in C++ (must match Python name) */
                                /* Argument(s) */
        );
    }

    bool py_ff_send_out(py::object task, int id = -1,
        unsigned long retry = ((unsigned long)-1),
        unsigned long ticks = (TICKS2WAIT)) {
        task.inc_ref();
        return ff_send_out((void*) ((PyObject*) task.ptr()), id, retry, ticks);
    }
};

class py_ff_minode: public ff_minode_wrapper {
public:
    /* Inherit the constructors */
    using ff_minode_wrapper::ff_minode_wrapper;

    py::object svc(py::object obj) override {
        /* PYBIND11_OVERRIDE_PURE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE_PURE(
            py::object,         /* Return type */
            ff_minode_wrapper,  /* Parent class */
            svc,                /* Name of function in C++ (must match Python name) */
            obj                 /* Argument(s) */
        );
    }

    void eosnotify(ssize_t id = -1) override {
        /* PYBIND11_OVERRIDE will acquire the GIL before accessing Python state */
        PYBIND11_OVERRIDE(
            void,               /* Return type */
            ff_minode_wrapper,  /* Parent class */
            eosnotify,          /* Name of function in C++ (must match Python name) */
            id                  /* Argument(s) */
        );
    }
};

PYBIND11_MODULE(fastflow, m) {
    m.doc() = "FastFlow, a pattern-based parallel programming framework.";
    
    m.attr("STOP")  = PY_STOP;
    m.attr("GO_ON") = PY_GO_ON;
    m.attr("EOS")   = PY_EOS;

    /* ff_pipeline */
    py::class_<ff::ff_pipeline>(m, "ff_pipeline")
        .def(py::init<>(), R"mydelimiter(
            The ff_pipeline constructor
        )mydelimiter")
        .def(
            "add_stage",
            static_cast<int (ff::ff_pipeline::*)(py_ff_node *, bool)>(&ff::ff_pipeline::add_stage),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage",
            static_cast<int (ff::ff_pipeline::*)(ff::ff_pipeline *, bool)>(&ff::ff_pipeline::add_stage),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage",
            static_cast<int (ff::ff_pipeline::*)(py_ff_minode *, bool)>(&ff::ff_pipeline::add_stage),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def("run_and_wait_end", &ff::ff_pipeline::run_and_wait_end, py::call_guard<py::gil_scoped_release>())
        .def("wrap_around", &ff::ff_pipeline::wrap_around)
        .def("ffTime", &ff::ff_pipeline::ffTime);
    
    /* ff_node */
    py::class_<ff_node_wrapper, py_ff_node>(m, "ff_node")
        .def(py::init<>())
        .def("svc", static_cast<py::object (ff_node_wrapper::*)(py::object)>(&ff_node_wrapper::svc), py::return_value_policy::automatic_reference)
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
    
    /* ff_minode */
    py::class_<ff_minode_wrapper, py_ff_minode>(m, "ff_minode")
        .def(py::init<>())
        .def("svc", static_cast<py::object (ff_minode_wrapper::*)(py::object)>(&ff_minode_wrapper::svc), py::return_value_policy::automatic_reference)
        .def("svc_init", &ff_minode_wrapper::svc_init)
        .def("svc_end", &ff_minode_wrapper::svc_end)
        .def(
            "ff_send_out", 
            &ff_minode_wrapper::py_ff_send_out,
            py::arg("task"),
            py::arg("id") = -1,
            py::arg("retry") = ((unsigned long)-1),
            py::arg("ticks") = (int) ff_minode_wrapper::TICKS2WAIT
        )
        .def("fromInput", &ff_minode_wrapper::fromInput)
        .def("eosnotify", &ff_minode_wrapper::eosnotify)
        .def("get_channel_id", &ff_minode_wrapper::get_channel_id);


#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
