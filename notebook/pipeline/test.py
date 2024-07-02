from fastflow_subint_module import FFPipeline

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

class sourcestage():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | source] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[{self.id} | source] svc, counter = {self.counter}, {args}')
        if self.counter == 6:
            return None
        self.counter += 1

        busy_work()

        return self.counter

    def svc_end(self):
        print(f'[{self.id} | source] svc_end was called')

class stage():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id}] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[{self.id}] svc, {args}')

        busy_work()

        # just forward
        return args

    def svc_end(self):
        print(f'[{self.id}] svc_end was called')

class sinkstage():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | sink] svc_init was called')
        return 0
    
    def svc(self, *args):
        print(f'[{self.id} | sink] svc, {args}')

        busy_work()

        return 0

    def svc_end(self):
        print(f'[{self.id} | sink] svc_end was called')

def build_pipe(nstages = 2, use_processes = True, use_subinterpreters = False):
    pipe = FFPipeline(use_subinterpreters)

    source = sourcestage("1")
    if use_processes:
        pipe.add_stage_process(source)
    else:
        pipe.add_stage(source)

    # count the sink and the source as stages
    for i in range(1, nstages - 1):
        innerstage = stage(f"{i+1}") # +1 beacuse i starts at 1 and 1 is the source
        if use_processes:
            pipe.add_stage_process(innerstage)
        else:
            pipe.add_stage(innerstage)
    
    sink = sinkstage(f"{nstages}")
    if use_processes:
        pipe.add_stage_process(sink)
    else:
        pipe.add_stage(sink)
    return pipe

def run_pipe(nstages, use_processes = False, use_subinterpreters = False):
    print(f"run pipe of {nstages} stages")
    pipe = build_pipe(nstages, use_processes, use_subinterpreters)
    pipe.run_and_wait_end()
    return pipe.ffTime()

"""processes = [[],[]]
for i in range(2, 6): # from 2 to 5
    res = run_pipe(i, use_processes = True, use_subinterpreters = False)
    print(f"done in {res}ms")
    processes[0].append(i) # x
    processes[1].append(res) # y

print("processes =", processes)"""

subinterpreters = [[],[]]
for i in range(2, 6): # from 2 to 5
    res = run_pipe(i, use_processes = False, use_subinterpreters = True)
    print(f"done in {res}ms")
    subinterpreters[0].append(i) # x
    subinterpreters[1].append(res) # y

print("subinterpreters =", subinterpreters)