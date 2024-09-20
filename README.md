## Usage

### Farm

```py
from fastflow import FFFarm, GO_ON
import sys

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *args):
        # args are sent by the previous stage 
        # or is an empty tuple if it is the first stage
        if self.counter == 12:
            return
        self.counter += 1

        return list(['source'])

class worker():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        print(lis)
        return GO_ON

# create a farm, pass True or False if you want to use subinterpreters or not
farm = FFFarm(False)
sourcenode = source()
sinknode = sink()
w_lis = []
for i in range(3):
    w = worker(f"{i+1}")
    w_lis.append(w)
farm.add_emitter(sourcenode)
farm.add_workers(w_lis)
farm.add_collector(sinknode)
# finally run the farm. Blocking call: will resume when the farm ends
farm.run_and_wait_end()

```

### Pipeline

```py
from fastflow import FFPipeline, GO_ON

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
        return GO_ON

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
print(f"pipeline done in {farm.ffTime()}ms")
```