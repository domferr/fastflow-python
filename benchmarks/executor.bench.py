import busy_wait
import argparse
import time
import concurrent.futures
import numpy
from fastflow import FastFlowExecutor, FFFarm, ff_send_out, EOS

def get_data_sample(task_bytes):
    from string import ascii_letters
    result = ascii_letters * int(task_bytes / len(ascii_letters))
    return result
    
def task_body(ms, data_sample):
    busy_wait.wait(ms)

def numpy_task(A, B):
    numpy.dot(A, B)

class emitter():
    def __init__(self, ntasks, data_sample):
        self.ntasks = ntasks
        self.data_sample = data_sample
    
    def svc(self, *args):
        for _ in range(self.ntasks):
            ff_send_out(self.data_sample)
        return EOS

class worker():
    def __init__(self, ms):
        self.ms = ms
    
    def svc(self, *args):
        task_body(self.ms, None)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a farm of <WORKERS> workers and <TASKS> tasks. Each task is <MS>ms long and has a size of <BYTES> bytes. Using subinterpreters or multiprocessing based strategy')
    parser.add_argument('-tasks', type=int, help='Number of tasks to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=True)
    parser.add_argument('-ms', type=int, help='Duration, in milliseconds, of one task', required=True)
    parser.add_argument('-bytes', type=int, help='The size, in bytes, of one task', required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-ffproc', action='store_true', help='Use FastFlow\'s multiprocessing to process tasks')
    group.add_argument('-ffsub', action='store_true', help='Use FastFlow\'s subinterpreters to process tasks')
    group.add_argument('-threadpool', action='store_true', help='Use concurrent.futures.ThreadPoolExecutor to process tasks')
    group.add_argument('-processpool', action='store_true', help='Use concurrent.futures.ProcessPoolExecutor to process tasks')
    group.add_argument('-fffarm', action='store_true', help='Use FFFarm to process tasks')
    args = parser.parse_args()

    # test the serialization to adjust the number of bytes
    data_sample = get_data_sample(args.bytes)
    start = time.clock_gettime_ns(time.CLOCK_MONOTONIC) 
    if args.fffarm:
        farm = FFFarm()
        farm.add_emitter(emitter(args.tasks, data_sample))
        farm.add_workers([worker(args.ms) for _ in range(args.workers)])
        farm.run_and_wait_end()
    else:
        if args.threadpool:
            exe = concurrent.futures.ThreadPoolExecutor(max_workers=args.workers)
        elif args.processpool:
            exe = concurrent.futures.ProcessPoolExecutor(max_workers=args.workers)
        elif args.ffproc:
            exe = FastFlowExecutor(max_workers=args.workers)
        elif args.ffsub:
            exe = FastFlowExecutor(max_workers=args.workers, use_subinterpreters=True)

        with exe:
            futures = []
            for _ in range(args.tasks):
                N = 2000
                # Create two large random matrices
                A = numpy.random.rand(N, N)
                B = numpy.random.rand(N, N)
                futures.append(exe.submit(numpy_task, A, B))
            #futures = [exe.submit(numpy.dot, A, B) for _ in range(args.tasks)]
            concurrent.futures.wait(futures)
    end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    print((end - start)/1000000000)