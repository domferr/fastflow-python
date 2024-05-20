#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/eval.h>
#include <iostream>

#include <string>
#include <vector>
#include <thread>
#include <iostream>

namespace py = pybind11;

class node
{
private:
    std::thread th;
    PyThreadState* sub_interpreter_ts;

public:
    void threadbody(PyInterpreterState* interp) {
        // create a new thread state for the sub interpreter
        PyThreadState* _ts = PyThreadState_New(interp);
        // make it the current thread state and acquire the sub interpreter's GIL
        PyEval_RestoreThread(_ts);

        // swap state to subinterpreter's one
        PyThreadState* _old_ts = PyThreadState_Swap(_ts);

        // run the initialization code. it will define the user function
        //PyRun_SimpleString(initcode->c_str());

        /*// Evaluate in scope of main module
        py::object scope = py::module_::import("__main__").attr("__dict__");
        */

        //PyRun_SimpleString("svc(17)");
        /*PyObject *globals = PyEval_GetGlobals();
        PyObject *result = PyRun_String("svc(17)", Py_eval_input, globals, globals);
        
        if (result == nullptr)
            throw py::error_already_set();
        Py_DECREF(result);*/

        py::object svc_fun = py::module_::import("__main__").attr("svc");
        py::object result = svc_fun(21);
        std::cout << "cpp return value is " << result.cast<int>() << std::endl;
        
        //py::object result = py::eval("svc(17)");

        // restore the original thread state
        PyThreadState_Swap(_old_ts);

        // clear the sub interpreter thread state
        PyThreadState_Clear(_ts);

        // delete the current thread state and release the sub interpreter's GIL
        PyThreadState_DeleteCurrent();
    }

    void run(std::string funSourceCode, std::string funName) {
        std::string* initcode = new std::string(R"PY(
FUN_DEF

def svc(*args):
    return FUN_NAME(args)
        )PY");

        initcode->replace(initcode->find("FUN_DEF"), 7, funSourceCode.c_str());
        initcode->replace(initcode->find("FUN_NAME"), 8, funName.c_str());

        // save the main interpreter state
        PyThreadState* _main_ts = PyThreadState_Get();

        // configuration for the sub interpeter. It will have its own GIL
        PyInterpreterConfig config = {
            .check_multi_interp_extensions = 1,
            .gil = PyInterpreterConfig_OWN_GIL//PyInterpreterConfig_SHARED_GIL,
        };
        
        // create a new interpreter
        // the sub-interpreter is created with its own GIL then the main interpreter's GIL
        // will be released. When Py_NewInterpreterFromConfig returns, the new interpreter’s GIL 
        // will be held by the current thread and the main interpreter’s GIL will remain released
        PyStatus status = Py_NewInterpreterFromConfig(&sub_interpreter_ts, &config);
        if (PyStatus_Exception(status)) {
            std::cout << "--------- EXCEPTION: PyStatus_Exception" << std::endl;
            
            // restore the main interpreter state
            PyThreadState_Swap(_main_ts);
            return;
        }

        PyRun_SimpleString(initcode->c_str());

        th = std::thread(&node::threadbody, this, sub_interpreter_ts->interp);
        
        // restore the main interpreter state
        PyThreadState_Swap(_main_ts);
    }

    void join() {
        py::gil_scoped_release release;
        th.join();
        py::gil_scoped_acquire acquire;
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

PYBIND11_MODULE(parallel, m) {
    m.doc() = "Parallel";

    py::class_<node>(m, "node")
        .def(py::init<>())
        .def("run", &node::run)
        .def("join", &node::join);


#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}