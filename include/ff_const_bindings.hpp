#ifndef FF_CONST_BINDINGS_HPP
#define FF_CONST_BINDINGS_HPP

#include <pybind11/pybind11.h>
#include <ff/ff.hpp>

namespace py = pybind11;

typedef struct ff_const {
    void * value;
} ff_const;

void ff_const_bindings(py::module_ &m) {
    py::class_<ff_const>(m, "ff_const");

    m.attr("STOP")  = ff_const{ NULL };
    m.attr("GO_ON") = ff_const{ ff::FF_GO_ON };
    m.attr("EOS")   = ff_const{ ff::FF_EOS };
}

#endif // FF_CONST_BINDINGS_HPP