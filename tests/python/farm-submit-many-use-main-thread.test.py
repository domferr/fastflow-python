from fastflow import FFFarm, EOS
import sys

"""
                  _ worker _
                 |          |
                 |_ worker _|
    source ------|          |--- sink (on main thread)
(on main thread) |_ worker _|
                 |          |
                 |_ worker _|
"""

class emitter():    
    def svc(self, lis: list):
        lis.append("emitter")
        return lis

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
    farm.add_workers([worker(f"worker{i+1}") for i in range(3)])
    farm.add_emitter(emitter(), use_main_thread=True)
    farm.add_collector(sink(), use_main_thread=True)
    
    farm.run()
    for i in range(8):
        farm.submit([f"submitted{i}"])
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