cmake_minimum_required(VERSION 3.4...3.18)
project(fastflow)
SET(CMAKE_CXX_STANDARD 20)

include_directories(extern/fastflow)

add_subdirectory(extern/pybind11)
pybind11_add_module(fastflow bindings.cpp ff_const_bindings.hpp ff_pipeline_bindings.hpp ff_node_bindings.hpp ff_minode_bindings.hpp ff_monode_bindings.hpp)

# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(fastflow PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
