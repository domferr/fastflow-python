import os
from setuptools import setup, Extension

def main():
    setup(name="busy_wait",
          language="c++",
          version="1.0.0",
          description="Description",
          author="Domenico Ferraro",
          author_email="ferraro.domenico125@gmail.com",
          include_dirs=[],
          ext_modules=[Extension("busy_wait", ["busy_wait.cpp"], language='c++')])


if __name__ == "__main__":
    os.environ["CC"] = "g++"
    main()