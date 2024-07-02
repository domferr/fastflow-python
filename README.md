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
