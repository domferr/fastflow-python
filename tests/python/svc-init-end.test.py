from fastflow import FFFarm, EOS
import sys

class node():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f"svc_init-{self.id}")

    def svc(self, *args):
        print(f"svc-{self.id}")
        return EOS
    
    def svc_end(self):
        print(f"svc_end-{self.id}")

def run_test(use_subinterpreters, use_main_thread = False):
    farm = FFFarm(use_subinterpreters)
    farm.add_workers([node(i+1) for i in range(4)], use_main_thread=use_main_thread)
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