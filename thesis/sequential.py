import argparse
from concurrent.futures import ThreadPoolExecutor, wait
import threading
import time
import busy_wait

def task():
    busy_wait.wait(100)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('--workers', type=int, help='Number of worker threads to use', required=True)
    args = parser.parse_args()
    start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

    with ThreadPoolExecutor(max_workers=args.workers) as exe:
        # issue tasks to the thread pool
        futures = [exe.submit(task) for _ in range(64) ]
        wait(futures)

    end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    print(args.workers, ((end - start)/1000))