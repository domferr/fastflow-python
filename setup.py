from setuptools import setup, Extension

def main():
    setup(
        name="fastflow_module",
        version="1.0.0",
        description="Description",
        author="Domenico Ferraro",
        author_email="ferraro.domenico125@gmail.com",
        python_requires=">=3.8",
        ext_modules=[
            Extension(
                "fastflow_module", 
                ["fastflow_module.cpp"],
                include_dirs=['fastflow', 'include'],
                language="c++",
                extra_compile_args=['-O3', '-std=c++17'],
                # use a safe subset of the Python C API to get a forward-compatibility guarantee 
                # to support any future version of Python, without recompilation
                py_limited_api = True
            )
        ],
    )

if __name__ == "__main__":
    main()
