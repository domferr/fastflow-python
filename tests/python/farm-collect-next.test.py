from fastflow import FFFarm, EOS
import sys

"""
    _ worker _
        
    _ worker _
    
    _ worker _
    
    _ worker _
"""

class worker():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

def run_test(use_subinterpreters = True):
    farm = FFFarm(use_subinterpreters)
    w_lis = []
    for i in range(3):
        w = worker(f"worker{i+1}")
        w_lis.append(w)
    farm.add_workers(w_lis)
    farm.run()
    for i in range(8):
        farm.submit([f"data{i}"])
    farm.submit(EOS)
    farm.wait()

    lis = farm.collect_next()
    while lis is not None:
        print(lis)
        lis = farm.collect_next()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)