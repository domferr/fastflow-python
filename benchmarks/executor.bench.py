import busy_wait
import argparse
import time
import concurrent.futures
from fastflow import FastFlowExecutor, FastFlowFarmExecutor
import numpy

def get_data_sample(task_bytes):
    from string import ascii_letters
    result = ascii_letters * int(task_bytes / len(ascii_letters))
    return result
    
def task_body(ms, data_sample):
    busy_wait.wait(ms)

def numpy_task(A, B):
    C = numpy.dot(A, B)
    #busy_wait.wait(25)

def numpy_task2(N):
    # Create two large random matrices
    A = numpy.random.rand(N, N)
    B = numpy.random.rand(N, N)
    C = numpy.dot(A, B)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a farm of <WORKERS> workers and <TASKS> tasks. Each task is <MS>ms long and has a size of <BYTES> bytes. Using subinterpreters or multiprocessing based strategy')
    parser.add_argument('-tasks', type=int, help='Number of tasks to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=True)
    parser.add_argument('-bytes', type=int, help='The size, in bytes, of one task', required=True)
    group2 = parser.add_mutually_exclusive_group(required=False)
    group2.add_argument('-numpy', action='store_true', help='The task uses numpy')
    group2.add_argument('-ms', type=int, help='Duration, in milliseconds, of one task')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-ffproc', action='store_true', help='Use FastFlow\'s multiprocessing to process tasks')
    group.add_argument('-ffsub', action='store_true', help='Use FastFlow\'s subinterpreters to process tasks')
    group.add_argument('-threadpool', action='store_true', help='Use concurrent.futures.ThreadPoolExecutor to process tasks')
    group.add_argument('-processpool', action='store_true', help='Use concurrent.futures.ProcessPoolExecutor to process tasks')
    group.add_argument('-fffarm', action='store_true', help='Use FFFarm to process tasks')
    group.add_argument('-fffarmsub', action='store_true', help='Use FFFarm and subinterpreters to process tasks')
    args = parser.parse_args()

    # test the serialization to adjust the number of bytes
    data_sample = get_data_sample(args.bytes)
    start = time.clock_gettime_ns(time.CLOCK_MONOTONIC) 
    if args.threadpool:
        exe = concurrent.futures.ThreadPoolExecutor(max_workers=args.workers)
    elif args.processpool:
        exe = concurrent.futures.ProcessPoolExecutor(max_workers=args.workers)
    elif args.ffproc:
        exe = FastFlowExecutor(max_workers=args.workers)
    elif args.ffsub:
        exe = FastFlowExecutor(max_workers=args.workers, use_subinterpreters=True)
    elif args.fffarm:
        exe = FastFlowFarmExecutor(max_workers=args.workers)
    elif args.fffarmsub:
        exe = FastFlowFarmExecutor(max_workers=args.workers, use_subinterpreters=True)
    
    with exe:
        if args.numpy:
            futures = []
            N = 500
            for _ in range(args.tasks):
                futures.append(exe.submit(numpy_task2, N))
        else:
            futures = [exe.submit(task_body, args.ms, data_sample) for _ in range(args.tasks)]

    concurrent.futures.wait(futures)

    end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    print((end - start)/1000000000, end='')