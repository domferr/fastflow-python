from fastflow_subint_module import FFPipeline

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

class stage():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc_init(self):
        print(f'[{self.id}] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[{self.id}] svc, counter = {self.counter}, {args}')
        if self.counter == 6:
            return None
        self.counter += 1

        busy_work()

        return self.counter

    def svc_end(self):
        print(f'[{self.id}] svc_end was called')

class sinkstage():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id}] svc_init was called')
        return 0
    
    def svc(self, *args):
        print(f'[{self.id}] svc, {args}')

        busy_work()

        return 0

    def svc_end(self):
        print(f'[{self.id}] svc_end was called')

def build_pipe(use_processes = True, use_subinterpreters = False):
    pipe = FFPipeline(use_subinterpreters)
    stage1 = stage("1")
    if use_processes:
        pipe.add_stage_process(stage1)
    else:
        pipe.add_stage(stage1)
    stage2 = sinkstage("2")
    if use_processes:
        pipe.add_stage_process(stage2)
    else:
        pipe.add_stage(stage2)
    return pipe

pipe1 = build_pipe(True, False)
pipe1.run_and_wait_end()
withProcessTime = pipe1.ffTime()
print(f"with processes: {withProcessTime}ms")
print()

pipe2 = build_pipe(False, True)
pipe2.run_and_wait_end()
withoutProcessTime = pipe2.ffTime()
print(f"with subinterpreters: {withoutProcessTime}ms")

pipe2 = build_pipe(False, False)
pipe2.run_and_wait_end()
withoutProcessTime = pipe2.ffTime()
print(f"standard: {withoutProcessTime}ms")

"""
with processes: 7534.448ms
with subinterpreters: 8010.326ms
standard: 13398.703ms
"""