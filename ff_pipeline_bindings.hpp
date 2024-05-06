#ifndef FF_PIPELINE_BINDINGS_HPP
#define FF_PIPELINE_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>
#include "ff_node_bindings.hpp"
#include "ff_minode_bindings.hpp"

namespace py = pybind11;

void ff_pipeline_bindings(py::module_ &m) {
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
}

#endif // FF_PIPELINE_BINDINGS_HPP