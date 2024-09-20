from fastflow import FFFarm, EOS, GO_ON
import argparse
import sys
import busy_wait

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

    def svc(self, *args):
        if self.n_tasks == 0:
            return EOS
        self.n_tasks -= 1

        return DummyData(self.data_sample)

class worker():
    def __init__(self, ms, id):
        self.ms = ms
        self.id = id

    def svc(self, *args):        
        busy_wait.wait(self.ms)
        return args

class collector():    
    def svc(self, *args):
        return GO_ON

def build_farm(n_tasks, task_ms, nworkers, data_sample, use_subinterpreters = False, use_main_thread = False):
    farm = FFFarm(use_subinterpreters)

    # emitter
    em = emitter(data_sample, n_tasks)
    farm.add_emitter(em, use_main_thread = use_main_thread)

    # build workers list
    w_lis = []
    for i in range(nworkers):
        w = worker(task_ms, f"{i+1}")
        w_lis.append(w)
    
    # add workers
    farm.add_workers(w_lis, use_main_thread = use_main_thread)

    # collector
    coll = collector()
    farm.add_collector(coll, use_main_thread = use_main_thread)

    return farm

def run_farm(n_tasks, task_ms, nworkers, data_sample, data_size, use_subinterpreters = False, blocking_mode = None, no_mapping = False, use_main_thread = False):
    strategy = "Not using any strategy" if use_main_thread else f"Using {'subinterpreters' if use_subinterpreters else 'processes'}-based strategy"
    print(f"Running a farm of {nworkers} workers and {n_tasks} tasks. Each task is {task_ms}ms long and has a size of {data_size} bytes. {strategy}", file=sys.stderr)
    farm = build_farm(n_tasks, task_ms, nworkers, data_sample, use_subinterpreters, use_main_thread)
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

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a farm of <WORKERS> workers and <TASKS> tasks. Each task is <MS>ms long and has a size of <BYTES> bytes. Using subinterpreters or multiprocessing based strategy')
    parser.add_argument('-tasks', type=int, help='Number of tasks to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=True)
    parser.add_argument('-ms', type=int, help='Duration, in milliseconds, of one task', required=True)
    parser.add_argument('-bytes', type=int, help='The size, in bytes, of one task', required=True)
    parser.add_argument('-blocking-mode', help='Enable blocking mode of the farm', required=False, action='store_true', default=None)
    parser.add_argument('-no-mapping', help="Disable fastflow's mapping", required=False, action='store_true')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    group.add_argument('-seq', action='store_true', help='Run tasks sequentially')
    args = parser.parse_args()

    # test the serialization to adjust the number of bytes
    data_sample = get_data_sample(args.bytes)
    
    res = run_farm(args.tasks, args.ms, args.workers, data_sample, args.bytes, use_subinterpreters = args.sub and not args.proc, blocking_mode = args.blocking_mode, no_mapping = args.no_mapping, use_main_thread = args.seq)
    print(f"res[{args.bytes}] = res.get({args.bytes}, []); res.get({args.bytes}).append(({args.workers}, {res})) # bytes =", args.bytes, "ms =", args.ms)


"""
Some examples to run this benchmark
- python3.12 benchmark/farm/bench.py -tasks 128 -workers 4 -ms 100 -bytes 512000 -sub
- python3.12 benchmark/farm/bench.py -tasks 128 -workers 4 -ms 100 -bytes 320024 -sub 2>1 | grep subinterp
- for i in 1 2 4 8 10 12 16 20 26 30 36 42 48 54 60 64; do for size in 1024 4096 8192 16384 32768 65536 524288 1048576; do python3.12 benchmark/farm/bench.py -tasks 512 -workers $i -ms 500 -bytes $size -sub 2>1 | grep subinterp; done; done
"""