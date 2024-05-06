# Just a pipeline with two stages, but the values sent by the first stage
# are objects or tuples.
#
#   Stage1 -----> Stage2

import time
from datetime import datetime
from fastflow import ff_pipeline, ff_node, STOP, GO_ON

def active_wait(ms):
    current_time = time.time_ns() // 1_000_000
    while ((time.time_ns() // 1_000_000) < ms+current_time):
        pass

def log(*args, **kwargs):
    print(f"[{datetime.now().strftime('%H:%M:%S.%f')}]", *args, **kwargs, flush=True)

class Dummy:
    def __init__(self, value):
        log('Dummy created.')
        self.value = value

    def __str__(self) -> str:
        return f"{self.value}"
    
    __repr__ = __str__

class Stage1(ff_node):
    def __init__(self):
        ff_node.__init__(self)
        self.counter = 1

    def svc(self, args):
        if self.counter > 5:
            return STOP
        
        if self.counter > 1:
            active_wait(500)

        # send object if counter is even, a tuple otherwise
        next_value = Dummy(self.counter) if self.counter % 2 == 0 else (self.counter, self.counter/2, Dummy(self.counter))
        log("Send", next_value, "to next stage")

        self.counter = self.counter + 1
        return next_value
    
    def svc_init(self):
        log("init first stage")
        return 0
    
    def svc_end(self):
        log("first stage ended")
    
class Stage2(ff_node):
    def __init__(self):
        ff_node.__init__(self)

    def svc_init(self):
        log("init second stage")
        return 0

    def svc(self, args):
        log("stage 2 got:", args, "-> type", type(args))
        active_wait(1500)
        return GO_ON
    
    def svc_end(self):
        log("second stage ended")

if __name__ == "__main__":
    stage1 = Stage1()
    stage2 = Stage2()
    print(stage1)
    print(stage2)
    
    pipe = ff_pipeline()
    print(pipe)
    
    pipe.add_stage(stage1)
    pipe.add_stage(stage2)
    pipe.run_and_wait_end()
    log("DONE", pipe.ffTime(), "(ms)")
    
