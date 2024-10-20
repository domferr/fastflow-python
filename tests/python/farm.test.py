from fastflow import FFFarm, EOS
import sys

"""
             _ worker _
            |          |
            |_ worker _|
  source ---|          |--- sink
            |_ worker _|
            |          |
            |_ worker _|
"""

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter == 12:
            return EOS
        self.counter += 1

        return list(['source'])

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
    sourcenode = source()
    sinknode = sink()
    w_lis = []
    for i in range(3):
        w = worker(f"worker{i+1}")
        w_lis.append(w)
    farm.add_emitter(sourcenode)
    farm.add_workers(w_lis)
    farm.add_collector(sinknode)
    farm.run_and_wait_end()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)