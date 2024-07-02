from fastflow_subint_module import FFFarm

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

class emitter():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *args):
        print(f'[{self.id} | emitter] svc, counter = {self.counter}, {args}')
        if self.counter == 6:
            return None
        self.counter += 1

        busy_work()

        return self.counter

    def svc_end(self):
        print(f'[{self.id} | emitter] svc_end was called')

class worker():
    def __init__(self, id):
        self.id = id

    def svc(self, *args):
        print(f'[{self.id} | worker] svc, {args}')

        busy_work()

        return args

    def svc_end(self):
        print(f'[{self.id} | worker] svc_end was called')

class collector():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | collector] svc_init was called')
        return 0
    
    def svc(self, *args):
        print(f'[{self.id} | collector] svc, {args}')

        busy_work()

        return 0

def build_farm(nworkers = 1, use_subinterpreters = True):
    farm = FFFarm(use_subinterpreters)
    em = emitter("1")
    farm.add_emitter(em)
    coll = collector("2")
    farm.add_collector(coll)

    w_lis = []
    for i in range(nworkers+1):
        w = worker(f"{i+1}")
        w_lis.append(w)
    farm.add_workers(w_lis)
    return farm

farm1 = build_farm(1, True)
farm1.run_and_wait_end()
withSubintTime = farm1.ffTime()
print(f"with subinterpreters: {withSubintTime}ms")
print()

farm2 = build_farm(1, False)
farm2.run_and_wait_end()
withoutSubintTime = farm2.ffTime()
print(f"without subinterpreters: {withoutSubintTime}ms")