import argparse
import threading
import time

def cpu_bound_task():
    count = 0
    for _ in range(10**7):
        count += 1

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('--ntasks', type=int, help='Number of tasks to process', required=True)
    args = parser.parse_args()

    start = time.time()
    threads = []
    for _ in range(args.ntasks):
        thread = threading.Thread(target=cpu_bound_task)
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()
    
    elapsed_time = time.time() - start
    print(args.ntasks, elapsed_time)