from fastflow_subint_module import FFPipeline

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

class stage():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *args):
        print(f'[{self.id}] svc, counter = {self.counter}, {args}')
        if self.counter == 6:
            return None
        self.counter += 1

        busy_work()

        return self.counter

    #def svc_end(self):
    #    print(f'[{self.id}] svc_end was called')

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

def build_pipe(use_subinterpreters = True):
    pipe = FFPipeline(use_subinterpreters)
    stage1 = stage("1")
    pipe.add_stage(stage1)
    stage2 = sinkstage("2")
    pipe.add_stage(stage2)
    return pipe

pipe = build_pipe(False)
pipe.run_and_wait_end()
print("DONE, time =", pipe.ffTime(), "(ms)")