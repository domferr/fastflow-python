import time
from fastflow_module import FFFarm
import sys

class DummyData:
    def __init__(self, val):
        self.astr = f"a string {val}"
        self.num = 1704 + val
        self.adict = {
            "val": 17 + val,
            "str": f"string inside a dictionary {val}"
        }
        self.atup = (4, 17, 16, val, 15, 30)
        self.aset = set([1, 2, 9, 6, val])
    
    def __str__(self):
        return f"{self.astr}, {self.num}, {self.adict}, {self.atup}, {self.aset}"
    
    __repr__ = __str__

def busy_work():
    time.sleep(2) # 2 seconds

class emitter():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | emitter] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[{self.id} | emitter] svc, counter = {self.counter}, {args}')
        if self.counter == 6:
            return None
        self.counter += 1

        busy_work()

        return DummyData(self.counter)

    def svc_end(self):
        print(f'[{self.id} | emitter] svc_end was called')

class worker():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | worker] svc_init was called')
        return 0

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

    def svc_end(self):
        print(f'[{self.id} | collector] svc_end was called')

def build_farm(nworkers = 1, use_processes = True, use_subinterpreters = False):
    farm = FFFarm(use_subinterpreters)

    # emitter
    em = emitter("1")
    if use_processes:
        farm.add_emitter_process(em)
    else:
        farm.add_emitter(em)

    # collector
    coll = collector("2")
    if use_processes:
        farm.add_collector_process(coll)
    else:
        farm.add_collector(coll)

    # build workers list
    w_lis = []
    for i in range(nworkers):
        w = worker(f"{i+1}")
        w_lis.append(w)
    
    # add workers
    if use_processes:
        farm.add_workers_process(w_lis)
    else:
        farm.add_workers(w_lis)

    return farm

def run_farm(nworkers, use_processes = False, use_subinterpreters = False):
    print(f"run farm of {nworkers} workers", file=sys.stderr)
    farm = build_farm(nworkers, use_processes, use_subinterpreters)
    farm.run_and_wait_end()
    return farm.ffTime()

"""processes = [[],[]]
for i in range(2, 6): # from 2 to 5
    res = run_farm(i, use_processes = True, use_subinterpreters = False)
    print(f"done in {res}ms")
    processes[0].append(i) # x
    processes[1].append(res) # y

print("processes =", processes)"""

"""standard = [[],[]]
for i in range(2, 6): # from 2 to 5
    res = run_farm(i, use_processes = False, use_subinterpreters = False)
    print(f"done in {res}ms")
    standard[0].append(i) # x
    standard[1].append(res) # y

print("standard =", standard)"""

subinterpreters = [[],[]]
for i in range(2, 6): # from 2 to 5
    res = run_farm(i, use_processes = False, use_subinterpreters = True)
    print(f"done in {res}ms")
    subinterpreters[0].append(i) # x
    subinterpreters[1].append(res) # y

print("subinterpreters =", subinterpreters)