from fastflow_module import FFFarm
import argparse
import sys
import math

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
    thousands = 245000 # 1000ms repara
    hundreds = 56950 # 100ms repara
    tens = 15370 # 10ms repara
    
    math.factorial(thousands)

class emitter():
    def __init__(self, n_tasks):
        self.n_tasks = n_tasks

    def svc_init(self):
        print(f'[emitter] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[emitter] svc, remaining tasks = {self.n_tasks}, {args}')
        if self.n_tasks == 0:
            return None
        self.n_tasks -= 1

        return DummyData(self.n_tasks)

    def svc_end(self):
        print(f'[emitter] svc_end was called')

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
    def svc_init(self):
        print(f'[collector] svc_init was called')
        return 0
    
    def svc(self, *args):
        print(f'[collector] svc, {args}')

        return args

    def svc_end(self):
        print(f'[collector] svc_end was called')

def build_farm(n_tasks, nworkers = 1, use_processes = True, use_subinterpreters = False):
    farm = FFFarm(use_subinterpreters)

    # emitter
    em = emitter(n_tasks)
    if use_processes:
        farm.add_emitter_process(em)
    else:
        farm.add_emitter(em)

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

    # collector
    coll = collector()
    if use_processes:
        farm.add_collector_process(coll)
    else:
        farm.add_collector(coll)

    return farm

def run_farm(n_tasks, nworkers, use_processes = False, use_subinterpreters = False):
    print(f"run farm of {nworkers} workers and {n_tasks} tasks", file=sys.stderr)
    farm = build_farm(n_tasks, nworkers, use_processes, use_subinterpreters)
    farm.run_and_wait_end()
    return farm.ffTime()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('num_tasks', type=int, help='Number of tasks to process')
    parser.add_argument('num_workers', type=int, help='Number of workers of the farm')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    args = parser.parse_args()

    tasks = list(range(args.num_tasks))

    if args.proc:
        processes = [[],[]]
        res = run_farm(args.num_tasks, args.num_workers, use_processes = True, use_subinterpreters = False)
        print(f"done in {res}ms")
        processes[0].append(args.num_workers) # x
        processes[1].append(res) # y

        print("processes =", processes)
    elif args.sub:
        subinterpreters = [[],[]]
        res = run_farm(args.num_tasks, args.num_workers, use_processes = False, use_subinterpreters = True)
        print(f"done in {res}ms")
        subinterpreters[0].append(args.num_workers) # x
        subinterpreters[1].append(res) # y

        print("subinterpreters =", subinterpreters)
    else:
        standard = [[],[]]
        res = run_farm(args.num_tasks, args.num_workers, use_processes = False, use_subinterpreters = False)
        print(f"done in {res}ms")
        standard[0].append(args.num_workers) # x
        standard[1].append(res) # y

        print("standard =", standard)
