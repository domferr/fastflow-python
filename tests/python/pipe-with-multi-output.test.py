from fastflow import FFAllToAll, FFPipeline, EOS, ff_send_out
import sys

"""
                __ first _   _ second
               |          | |
               |__ first _|_|_ second
    source --- |          | |
               |__ first _|_|_ second
               |          |
               |__ first _|
"""

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        ff_send_out(list(["source-to-first1"]), 0)
        return list(["source-to-any"])

class first():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class second():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        print(lis)

def run_test(use_subinterpreters = True):
    pipe = FFPipeline(use_subinterpreters)

    source_node = source()
    pipe.add_stage(source_node)

    a2a = FFAllToAll(use_subinterpreters)
    first_stage_size = 4
    second_stage_size = 3
    # build first stages
    first_lis = [first(f"first{i+1}") for i in range(first_stage_size)]
    # build second stages
    second_lis = [second(f"second{i+1}") for i in range(second_stage_size)]
    # add first stages
    a2a.add_firstset(first_lis)
    # add second stages
    a2a.add_secondset(second_lis)

    pipe.add_stage(a2a)
    
    pipe.run_and_wait_end()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)