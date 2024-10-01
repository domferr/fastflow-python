
import argparse
import time
from concurrent.futures import ProcessPoolExecutor, wait
from fastflow import FFFarm, EOS, ff_send_out
import os

print("disable numpy threads")
os.environ['OPENBLAS_NUM_THREADS'] = '1'
os.environ['MKL_NUM_THREADS'] = '1'
import numpy as np

class source():
    def __init__(self, ntasks, workers):
        self.ntasks = ntasks
        self.workers = workers

    def svc(self, *args):
        while self.ntasks > 0:
            self.ntasks -= 1
            N = 3000
            ff_send_out(N, self.ntasks % self.workers)
        return EOS

class worker():
    def svc(self, N):
        N = 3000
        # Create two large random matrices
        A = np.random.rand(N, N)
        B = np.random.rand(N, N)

        # Perform matrix multiplication
        C = np.dot(A, B)

def task():
    N = 3000
    # Create two large random matrices
    A = np.random.rand(N, N)
    B = np.random.rand(N, N)

    # Perform matrix multiplication
    C = np.dot(A, B)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('--ntasks', type=int, help='Number of tasks', required=True)
    parser.add_argument('--workers', type=int, help='Number of workers', required=True)
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-fastflow', action="store_true", help='Use FastFlow', required=False)
    group.add_argument('-processpool', action='store_true', help='Use ProcessPool from concurrent.futures')
    args = parser.parse_args()

    start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    if args.fastflow:
        farm = FFFarm()
        farm.no_mapping()
        farm.blocking_mode(True)
        farm.add_emitter(source(args.ntasks, args.workers))
        farm.add_workers([worker() for _ in range(args.workers)])
        farm.run_and_wait_end()
    elif args.processpool:
        with ProcessPoolExecutor(max_workers=args.workers) as exe:
            # issue tasks to the thread pool
            futures = [exe.submit(task) for _ in range(args.ntasks) ]
            wait(futures)
    else:
        for _ in range(args.ntasks):
            task()

    end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    print((end - start)/1000000000)