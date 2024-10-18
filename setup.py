from setuptools import setup, Extension

def main():
    setup(
        name="fastflow",
        version="1.0.0",
        description="Description",
        author="Domenico Ferraro",
        author_email="ferraro.domenico125@gmail.com",
        ext_modules=[
            Extension(
                "fastflow", 
                ["fastflow_module.cpp"],
                include_dirs=['extern/fastflow', 'include'],
                language="c++",
                extra_compile_args=['-O3', '-std=c++17'],
                # use a safe subset of the Python C API to get a forward-compatibility guarantee 
                # to support any future version of Python, without recompilation
                py_limited_api = True
            )
        ],
        package_dir={"": "."},
        python_requires=">=3.8",
    )

if __name__ == "__main__":
    main()
