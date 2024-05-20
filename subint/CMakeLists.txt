cmake_minimum_required(VERSION 3.4...3.18)

project(parallel)
SET(CMAKE_CXX_STANDARD 20)

#find_package(PythonLibs REQUIRED)
#include_directories(${PYTHON_INCLUDE_DIRS})

#set(SOURCE_FILES main.cpp)

#add_executable(py1 ${SOURCE_FILES})
#target_link_libraries(py1 ${PYTHON_LIBRARIES})

add_subdirectory(extern/pybind11)
pybind11_add_module(parallel parallelbindings.cpp)

# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(parallel PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})