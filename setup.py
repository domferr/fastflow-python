import os
from setuptools import setup, Extension

ext_lib_path = 'fastflow'

def main():
    setup(name="fastflow_module",
          language="c++",
          version="1.0.0",
          description="Description",
          author="Domenico Ferraro",
          author_email="ferraro.domenico125@gmail.com",
          include_dirs=[ext_lib_path, 'include'],
          ext_modules=[Extension("fastflow_module", ["fastflow_module.cpp"], language='c++')])


if __name__ == "__main__":
    os.environ["CC"] = "g++"
    main()
