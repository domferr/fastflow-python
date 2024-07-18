from fastflow_module import FFFarm
import argparse
import sys
import busy_wait

"""class DummyData:
    def __init__(self, size, val):
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
    
    __repr__ = __str__"""

class DummyData:
    def __init__(self, data):
        self.data = [data]

    def __str__(self):
        return f"{self.data}"
    
    __repr__ = __str__

class emitter():
    def __init__(self, data_sample, n_tasks):
        self.n_tasks = n_tasks
        self.data_sample = data_sample

    def svc_init(self):
        print('[emitter] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[emitter] svc, remaining tasks = {self.n_tasks}')
        if self.n_tasks == 0:
            return None
        self.n_tasks -= 1

        return DummyData(self.data_sample)

    def svc_end(self):
        print(f'[emitter] svc_end was called')

class worker():
    def __init__(self, ms, id):
        self.ms = ms
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | worker] svc_init was called')
        return 0

    def svc(self, *args):
        print(f'[{self.id} | worker] svc')
        
        busy_wait.wait(self.ms)

        return args

    def svc_end(self):
        print(f'[{self.id} | worker] svc_end was called')

class collector():
    def svc_init(self):
        print('[collector] svc_init was called')
        return 0
    
    def svc(self, *args):
        print('[collector] svc')

        return args

    def svc_end(self):
        print(f'[collector] svc_end was called')

def build_farm(n_tasks, task_ms, nworkers, data_sample, use_processes = True, use_subinterpreters = False):
    farm = FFFarm(use_subinterpreters)

    # emitter
    em = emitter(data_sample, n_tasks)
    if use_processes:
        farm.add_emitter_process(em)
    else:
        farm.add_emitter(em)

    # build workers list
    w_lis = []
    for i in range(nworkers):
        w = worker(task_ms, f"{i+1}")
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

def run_farm(n_tasks, task_ms, nworkers, data_sample, use_processes = False, use_subinterpreters = False, blocking_mode = None, no_mapping = False):
    print(f"run farm of {nworkers} workers and {n_tasks} tasks", file=sys.stderr)
    farm = build_farm(n_tasks, task_ms, nworkers, data_sample, use_processes, use_subinterpreters)
    if blocking_mode is not None:
        farm.blocking_mode(blocking_mode)
    if no_mapping:
        farm.no_mapping()
    farm.run_and_wait_end()
    return farm.ffTime()

def get_data_sample(task_bytes):
    from string import ascii_letters
    result = ascii_letters * int(task_bytes / len(ascii_letters))
    return result

def print_res(title, res, args):
    print(title)
    print(f"res[{args.bytes}] = res.get({args.bytes}, []); res.get({args.bytes}).append(({args.workers}, {res})) # bytes =", args.bytes, "ms =", args.ms)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('-tasks', type=int, help='Number of tasks to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=True)
    parser.add_argument('-ms', type=int, help='Duration in milliseconds of one task', required=True)
    parser.add_argument('-bytes', type=int, help='The size, in bytes, of one task', required=True)
    parser.add_argument('-blocking-mode', help='The blocking mode of the farm', required=False, action='store_true', default=None)
    parser.add_argument('-no-mapping', help="Disable fastflow's mapping", required=False, action='store_true')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    group.add_argument('-seq', action='store_true', help='Run tasks sequentially')
    args = parser.parse_args()

    # test the serialization to adjust the number of bytes
    data_sample = get_data_sample(args.bytes)

    if args.proc:
        processes = [[],[]]
        res = run_farm(args.tasks, args.ms, args.workers, data_sample, use_processes = True, use_subinterpreters = False, blocking_mode = args.blocking_mode, no_mapping = args.no_mapping)
        print(f"done in {res}ms")
        processes[0].append(args.workers) # x
        processes[1].append(res) # y

        print_res("processes", res, args)
    elif args.sub:
        subinterpreters = [[],[]]
        res = run_farm(args.tasks, args.ms, args.workers, data_sample, use_processes = False, use_subinterpreters = True, blocking_mode = args.blocking_mode, no_mapping = args.no_mapping)
        print(f"done in {res}ms")
        subinterpreters[0].append(args.workers) # x
        subinterpreters[1].append(res) # y

        print_res("subinterpreters", res, args)
    else:
        standard = [[],[]]
        res = run_farm(args.tasks, args.ms, args.workers, data_sample, use_processes = False, use_subinterpreters = False)
        print(f"done in {res}ms")
        standard[0].append(args.workers) # x
        standard[1].append(res) # y

        print_res("standard", res, args)


# for i in 1 2 4 8 10 12 16 20 26 30 36 42 48 54 60 64; do for size in 1024 4096 8192 16384 32768 65536 524288 1048576; do echo $i $size; done; done
# python3.12 benchmark/farm/bench.py -tasks 128 -workers 4 -ms 100 -bytes 512000 -sub
# python3.12 benchmark/farm/bench.py -tasks 128 -workers 4 -ms 100 -bytes 320024 -sub 2>1 | grep subinterp

# for i in 1 2 4 8 10 12 16 20 26 30 36 42 48 54 60 64; do for size in 1024 4096 8192 16384 32768 65536 524288 1048576; do python3.12 benchmark/farm/bench.py -tasks 512 -workers $i -ms 500 -bytes $size -sub 2>1 | grep subinterp; done; done