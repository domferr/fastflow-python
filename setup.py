from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            "fastflow._fastflow", 
            ["src/fastflow/_fastflow/_fastflowmodule.cpp"],
            include_dirs=['extern/fastflow', 'src/fastflow/_fastflow/include'],
            language="c++",
            extra_compile_args=['-O3', '-std=c++17'],
            # use a safe subset of the Python C API to get a forward-compatibility guarantee 
            # to support any future version of Python, without recompilation
            py_limited_api = True
        )
    ],
)
