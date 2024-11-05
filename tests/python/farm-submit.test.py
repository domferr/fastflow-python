from fastflow import FFFarm, EOS
import sys

"""
     _ worker _
    |          |
    |_ worker _|
    |          |--- sink
    |_ worker _|
    |          |
    |_ worker _|
"""

class worker():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        print(lis)

def run_test(use_subinterpreters = True):
    farm = FFFarm(use_subinterpreters)
    sinknode = sink()
    w_lis = []
    for i in range(3):
        w = worker(f"worker{i+1}")
        w_lis.append(w)
    farm.add_workers(w_lis)
    farm.add_collector(sinknode)
    farm.run()
    for i in range(8):
        farm.submit([f"data{i}"])
    farm.submit(EOS)
    farm.wait()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)