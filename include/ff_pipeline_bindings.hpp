#ifndef FF_PIPELINE_BINDINGS_HPP
#define FF_PIPELINE_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>
#include "ff_node_bindings.hpp"
#include "ff_minode_bindings.hpp"
#include "ff_monode_bindings.hpp"

namespace py = pybind11;

class subint_ff_node: public ff::ff_node
{
public:
    subint_ff_node(PyThreadState* sub_interpreter_ts): sub_interpreter_ts(sub_interpreter_ts), _ts(NULL), _old_ts(NULL) {}

    int svc_init() override {
        if (_ts != NULL || _old_ts != NULL || sub_interpreter_ts == NULL) return -1;

        // create a new thread state for the sub interpreter
        _ts = PyThreadState_New(sub_interpreter_ts->interp);
        // make it the current thread state and acquire the sub interpreter's GIL
        PyEval_RestoreThread(_ts);

        // swap state to subinterpreter's one
        _old_ts = PyThreadState_Swap(_ts);

        // run the preparation function to link ff_node's functions (e.g. ff_send_out) 
        // to the python's class equivalent functions. The python's prepare function will 
        // bind, for example, its ff_send_out function to the ff_node.ff_send_out function
        auto prepare = py::module_::import("__main__").attr("__obj").attr("_fastflow_prepare");
        prepare(this);

        return 0;
    }

    void * svc(void *arg) {
        py::object obj;
        if (arg != NULL) {
            // move operation. it keeps the same ref count instead of increasing
            obj = py::reinterpret_steal<py::object>((PyObject*) arg);
            std::cout << ff::ff_node::get_my_id() << ": arg ptr " << obj.ptr() << std::endl;
        }

        py::object svc_fun = py::module_::import("__main__").attr("__obj").attr("svc");
        // if the argument is NULL, pass nothing to python function
        py::object result = arg == NULL ? svc_fun():svc_fun(obj);
        std::cout << ff::ff_node::get_my_id() << ": result ptr " << result.ptr() << std::endl;

        py::object stop_value = py::module_::import("__main__").attr("STOP");
        if (result.is(stop_value)) {
            return NULL;
        }

        /*if (py::isinstance<ff_const>(result)) {
            ff_const* constant = py::cast<ff_const*>(result);
            return constant->value;
        }*/
        
        // since it was returned, its ref count was decreased
        // however it is now owned by c++ side
        // so increase ref count
        result.inc_ref();
        
        return (void*) ((PyObject*) result.ptr());
    }

    void svc_end() override {
        // restore the original thread state
        PyThreadState_Swap(_old_ts);

        // clear the sub interpreter thread state
        PyThreadState_Clear(_ts);
        _old_ts = NULL;
        _ts = NULL;

        // delete the current thread state and release the sub interpreter's GIL
        PyThreadState_DeleteCurrent();

        destroy();
    }

    ~subint_ff_node() {
        destroy();
    }

    bool py_ff_send_out(py::object& obj, int id = -1,
        unsigned long retry = ((unsigned long)-1),
        unsigned long ticks = (TICKS2WAIT)) {

        void * task = (void*) ((PyObject*) obj.ptr());

        py::object stop_value = py::module_::import("__main__").attr("STOP");
        if (obj.is(stop_value)) {
            task = NULL;
        }

        return ff_send_out(task, id, retry, ticks);
    }

private:
    PyThreadState* sub_interpreter_ts;
    PyThreadState* _ts;
    PyThreadState* _old_ts;

    void destroy() {
        if (sub_interpreter_ts) {
            // save the main interpreter's state
            PyThreadState* _main_ts = PyThreadState_Swap(sub_interpreter_ts);

            // end the subinterpreter
            Py_EndInterpreter(sub_interpreter_ts);
            sub_interpreter_ts = NULL;

            // restore the main interpreter's state
            PyThreadState_Swap(_main_ts);
        }
    }
};

subint_ff_node* to_ff_node(std::string serializedObject, std::string classSourceCode) {
    std::string* initcode = new std::string(R"PY(
class ff_const:
    def __init__(self, val):
        self.value = val

STOP = ff_const(0)

class Custom:
    def __init__(self, v):
        self.value1 = "this is a str"
        self.value2 = (10, v)

CLASS_SRC

    # add the _fastflow_prepare function to the class
    def _fastflow_prepare(self, arg):
        self.ff_send_out = arg.ff_send_out

import pickle
__obj = pickle.loads(SER_OBJ)
    )PY");

    initcode->replace(initcode->find("CLASS_SRC"), 10, classSourceCode.c_str());
    initcode->replace(initcode->find("SER_OBJ"), 7, serializedObject.c_str());

    // save the main interpreter state
    PyThreadState* _main_ts = PyThreadState_Get();

    // configuration for the sub interpeter. It will have its own GIL
    PyInterpreterConfig config = {
        .check_multi_interp_extensions = 1,
        .gil = PyInterpreterConfig_OWN_GIL//PyInterpreterConfig_SHARED_GIL,
    };
    
    PyThreadState* sub_interpreter_ts;
    // create a new interpreter
    // the sub-interpreter is created with its own GIL then the main interpreter's GIL
    // will be released. When Py_NewInterpreterFromConfig returns, the new interpreter’s GIL 
    // will be held by the current thread and the main interpreter’s GIL will remain released
    PyStatus status = Py_NewInterpreterFromConfig(&sub_interpreter_ts, &config);
    if (PyStatus_Exception(status)) {
        std::cout << "--------- EXCEPTION: PyStatus_Exception" << std::endl;
        
        // restore the main interpreter state
        PyThreadState_Swap(_main_ts);
        return NULL;
    }

    PyRun_SimpleString(initcode->c_str());
    free(initcode);

    // restore the main interpreter state
    PyThreadState_Swap(_main_ts);
    return new subint_ff_node(sub_interpreter_ts);
}

void ff_pipeline_bindings(py::module_ &m) {
    py::class_<subint_ff_node>(m, "subint_ff_node")
        .def(
            "ff_send_out", 
            &subint_ff_node::py_ff_send_out,
            py::keep_alive<1, 2>(),
            py::arg("task"),
            py::arg("id") = -1,
            py::arg("retry") = ((unsigned long)-1),
            py::arg("ticks") = (int) subint_ff_node::TICKS2WAIT
        );

    m.def("to_ff_node", &to_ff_node);

    py::class_<ff::ff_pipeline>(m, "ff_pipeline")
        .def(py::init<>(), R"mydelimiter(
            The ff_pipeline constructor
        )mydelimiter")
        .def(
            "add_stage", // pass a function to add a ff_node stage
            static_cast<int (ff::ff_pipeline::*)(subint_ff_node *, bool)>(&ff::ff_pipeline::add_stage),
            //py::keep_alive<1, 2>(),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage", // add ff_node stage
            static_cast<int (ff::ff_pipeline::*)(py_ff_node *, bool)>(&ff::ff_pipeline::add_stage),
            py::keep_alive<1, 2>(),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage", // add ff_pipeline stage
            static_cast<int (ff::ff_pipeline::*)(ff::ff_pipeline *, bool)>(&ff::ff_pipeline::add_stage),
            py::keep_alive<1, 2>(),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage", // add ff_minode stage
            static_cast<int (ff::ff_pipeline::*)(py_ff_minode *, bool)>(&ff::ff_pipeline::add_stage),
            py::keep_alive<1, 2>(),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def(
            "add_stage", // add ff_monode stage
            static_cast<int (ff::ff_pipeline::*)(py_ff_monode *, bool)>(&ff::ff_pipeline::add_stage),
            py::keep_alive<1, 2>(),
            py::arg("s"),
            py::arg("cleanup") = false, 
            R"mydelimiter(
                The add_stage function
            )mydelimiter")
        .def("run_and_wait_end", &ff::ff_pipeline::run_and_wait_end, py::call_guard<py::gil_scoped_release>())
        .def("wrap_around", &ff::ff_pipeline::wrap_around)
        .def("ffTime", &ff::ff_pipeline::ffTime);
}

#endif // FF_PIPELINE_BINDINGS_HPP