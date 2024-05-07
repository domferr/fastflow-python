## Contents
- [How to Build](https://github.com/domferr/fastflow-python#how-to-build)
- [How to Use](https://github.com/domferr/fastflow-python#how-to-use)

## To do
- [x] Bindings of **ff_pipeline** (main functions only)
- [x] Bindings of **ff_node** (main functions only)
- [x] Bindings of **ff_minode** (main functions only)
- [x] Bindings of **ff_monode** (main functions only)
- [x] Computation is parallel
- [x] svc function can support any python object of any type (**dynamic typing**)
- [x] Simple example with one ff_node
- [x] Example with ff_pipeline, ff_node and ff_minode
- [x] Example to check that any python object is not destroyed when referred by c++ code and not by python code
- [x] Partitioning bindings code over multiple files
- [x] If the argument passed to ff_pipeline.add_stage goes out of scope, the stage object must be kept alive
- [ ] **Documentation**
- [ ] Check if arrays can be used as argument and return value of svc function
- [ ] Create setup.py and pyproject.toml to build python module instead of manually using cmake
- [ ] Avoid C++ types in documentation
- [ ] How to unit test?
- [ ] Check if numpy can be used and if there are problems

## How to build

### Requirements
On Linux you’ll need to install the **python-dev** or **python3-dev** packages as well as cmake. On macOS, the included python version works out of the box, but **cmake** must still be installed.

Create your python virtual environment and activate it.
```
python3 -m venv .venv
source .venv/bin/activate
``` 

### Build
> It is a manual build since `setup.py` file is not ready yet

```bash
mkdir build/
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ../ && make && cp fastflow.cpython-311-x86_64-linux-gnu.so ../.venv/lib/python3.11/site-packages/
```

> `cp fastflow.cpython-311-x86_64-linux-gnu.so ../.venv/lib/python3.11/site-packages/` is optional. It is a manual installation of the created python module: when importing fastflow, the python interpreter will look into the site-packages folder and will find it. Otherwise, if you don't do that, you will need to have the file `fastflow.cpython-311-x86_64-linux-gnu.so` near the `.py` source code that imports it.

## How to use

1. Import fastflow
```python
from fastflow import ff_pipeline, ff_node, STOP, GO_ON
```

2. Create the first stage (a subclass of ff_node)
```python
class Stage1(ff_node):
    def __init__(self):
        ff_node.__init__(self)
    
    def svc_init(self):
        self.counter = 0
        return 0

    def svc(self, *args):
        value_to_next_stage = self.counter
        self.counter = self.counter + 1
        if self.counter > 10:
            return STOP
        
        print("Hi!")

        return value_to_next_stage
    
    def svc_end(self):
        print("first stage ended")
```

3. Create the second stage (a subclass of ff_node)
```python
class Stage2(ff_node):
    def __init__(self):
        ff_node.__init__(self)
    
    def svc_init(self):
        return 0

    def svc(self, *args):
        print("Got", args, "from previous stage")

        return GO_ON
    
    def svc_end(self):
        print("second stage ended")
```

4. Define a main function which runs the pipeline
```python
def main():
    # create a pipeline and the stage
    pipe = ff_pipeline() 
    stage1 = Stage1()
    stage2 = Stage2()
    pipe.add_stage(stage1) # add the first stage
    pipe.add_stage(stage2) # then add the second stage

    # blocking call: run the pipeline and wait for it to end
    if pipe.run_and_wait_end() < 0:
        raise("running pipeline") # throw exception if there is a failure
    
    # print stats
    print("DONE, time =", pipe.ffTime(), "(ms)")
    
if __name__ == "__main__":
    main()
```

> This is the example `examples/simplepipeline.py`. Can be run via `python3 examples/simplepipeline.py`
