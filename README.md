### TODO

- [x] Bindings of **ff_pipeline** (main functions only)
- [x] Bindings of **ff_node** (main functions only)
- [x] Bindings of **ff_minode** (main functions only
- [x] Computation is parallel
- [x] svc function can support any python object of any type (**dynamic typing**)
- [x] Simple example with one ff_node
- [x] Example with ff_pipeline, ff_node and ff_minode
- [x] Example to check that any python object is not destroyed when referred by c++ code and not by python code
- [ ] **Documentation**
- [ ] Check if arrays can be used as argument and return value of svc function
- [ ] Check what happens if ff_node/ff_minode/ff_pipeline subclasses goes out of scope
- [ ] Check what happens if the argument passed to ff_pipeline.add_stage goes out of scope
- [ ] Create setup.py and pyproject.toml to build python module instead of manually using cmake
- [ ] Avoiding C++ types in docstrings
- [ ] Partitioning bindings code over multiple files
- [ ] How to unit test?
- [ ] Check if numpy can be used and if there are problems

### Requirements:
- python3-dev

### Required Python packages:
- wheel
