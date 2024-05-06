#ifndef FF_CONST_BINDINGS_HPP
#define FF_CONST_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>

namespace py = pybind11;

auto PY_STOP  = py::cast(NULL);
auto PY_GO_ON = py::cast(ff::FF_GO_ON);
auto PY_EOS   = py::cast(ff::FF_EOS);

void ff_const_bindings(py::module_ &m) {
    m.attr("STOP")  = PY_STOP;
    m.attr("GO_ON") = PY_GO_ON;
    m.attr("EOS")   = PY_EOS;
}

#endif // FF_CONST_BINDINGS_HPP