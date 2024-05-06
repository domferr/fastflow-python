#include <pybind11/pybind11.h>
#include <ff/ff.hpp>
#include "ff_pipeline_bindings.hpp"
#include "ff_node_bindings.hpp"
#include "ff_minode_bindings.hpp"
#include "ff_const_bindings.hpp"

namespace py = pybind11;

PYBIND11_MODULE(fastflow, m) {
    m.doc() = "FastFlow, a pattern-based parallel programming framework.";
    
    /* fastflow constants */
    ff_const_bindings(m);

    /* ff_pipeline */
    ff_pipeline_bindings(m);
    
    /* ff_node */
    ff_node_bindings(m);
    
    /* ff_minode */
    ff_minode_bindings(m);


#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
