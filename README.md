## Usage

### Farm

```py
from fastflow_subint_module import FFFarm

# define an emitter
class emitter():
    def svc(self, *args):
        data = do_some_work()
        return data # data to send to workers

    def svc_end(self):
        do_some_work() # do some work when the emitter ends

class worker():
    def svc(self, *args): # args are sent by the emitter
        # perform some work
        data = busy_work(args)

        return data # data to send to collector

    def svc_end(self):
        do_some_work() # do some work when the worker ends

class collector():
    def svc_init(self):
        do_some_work() # do some work when the collector starts
        return 0 # 0 for success, error otherwise
    
    def svc(self, *args): # args are sent by the workers
        # perform some work
        busy_work(args)

        return 0


# create a farm, pass True or False if you want to use subinterpreters or not
farm = FFFarm(True)

# create and add emitter
em = emitter()
farm.add_emitter(em)

# create and add collector
coll = collector()
farm.add_collector(coll)

# create 4 workers and put them into a list
w_lis = []
for i in range(4):
    w = worker(f"{i+1}")
    w_lis.append(w)
# add the list of workers to the farm
farm.add_workers(w_lis)

# finally run the farm. Blocking call: will resume when the farm ends
farm.run_and_wait_end()
# print how many milliseconds the farm took
print(f"farm done in {farm.ffTime()}ms")
```

### Pipeline

```py
from fastflow_subint_module import FFPipeline

# define a stage
class stage():
    def svc(self, *args):
        # args are sent by the previous stage 
        # or is an empty tuple if it is the first stage
        data = do_some_work()
        return data # data to send to next stage

    def svc_end(self):
        do_some_work() # do some work when the stage ends

# define the last stage
class sinkstage():
    def svc_init(self):
        do_some_work() # do some work when the last stage starts
        return 0 # 0 for success, error otherwise
    
    def svc(self, *args): # args are sent by the previous stage
        data = do_some_work()
        return 0

# create a pipeline, pass True or False if you want to use subinterpreters or not
pipe = FFPipeline(use_subinterpreters)

# create and first stage
stage1 = stage()
pipe.add_stage(stage1)

# create and second stage
stage2 = stage()
pipe.add_stage(stage2)

# create and last stage
sink = sinkstage()
pipe.add_stage(sink)

# finally run the pipeline. Blocking call: will resume when the pipeline ends
pipe.run_and_wait_end()
# print how many milliseconds the pipeline took
print(f"farm done in {farm.pipeline()}ms")
```

## Open questions
- How to handle multi input/output nodes? The function ff_send_out(...) is not a member of the node class. A possible solution:
```
class worker():
    def svc(self, *args): # args are sent by the emitter
        # perform some work
        data = busy_work(args)
        fastflow.ff_send_out(data, 1) # <----- call ff_send_out
        return # the underline ff_node should continue instead of stopping

    def svc_end(self):
        do_some_work() # do some work when the worker ends
```
- Who frees the serialized objects in a farm? A solution "who deserializes then frees" is not suitable since N > 1 workers share the same serialized data of an object
- How to handle renaming of imported modules when recreating the environment in processes/subinterpreters? For example `import numpy as np` would cause the statement `import np` when recreating the environment.
- Can we use shared memory instead of pipes?
- If we use pipes, we need to handle the case of overflowing the pipe (it can be solved by calling the usual `readn` instead of `read` system call).
- Why calling `svc` inside subinterpreters is slower than calling it inside processes (including the time needed to send through pipe + recv ack + recv response + send ack)...?
- Memory leaks...
- Workers are added to the farm at the same time. The environment is serialized for each worker, but it may be just serialized once and shared accross all the workers. However it is not easy: if done during svc_init, how can we share the env accross all the workers and how we choose which worker does the serialization?
