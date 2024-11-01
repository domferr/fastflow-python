import concurrent.futures
from fastflow import FastFlowExecutor

def example():
    for _ in range(2**24):
        pass

if __name__ == "__main__":
    max_workers = 2
    tasks = 4

    with FastFlowExecutor(max_workers=max_workers) as exe:
        # issue tasks
        result = [exe.submit(example) for _ in range(tasks)]
        # wait for the tasks to complete
        concurrent.futures.wait(result)