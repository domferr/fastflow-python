from fastflow_module import FFAllToAll
import sys

"""
    first _   _ second
           | |
    first _|_|_ second
           | |
    first _|_|_ second
           |
    first _|
"""

class source():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *arg):
        if self.counter > 5:
            return None
        self.counter += 1

        return list([self.id])

class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        print(lis)
        return 0

def run_test(use_subinterpreters = True):
    a2a = FFAllToAll(use_subinterpreters)
    first_stage_size = 4
    second_stage_size = 3
    # build first stages
    first_lis = [source(i+1) for i in range(first_stage_size)]
    # build second stages
    second_lis = [sink(i+1) for i in range(second_stage_size)]
    # add first stages
    a2a.add_firstset(first_lis)
    # add second stages
    a2a.add_secondset(second_lis)
    a2a.run_and_wait_end()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)