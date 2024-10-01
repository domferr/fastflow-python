from fastflow import FFAllToAll, FFPipeline, EOS, ff_send_out
import sys

"""
                __ first _   _ second _                     __ first _   _ second
               |          | |          |                   |          | |
               |__ first _|_|_ second _|                   |__ first _|_|_ second
    source --- |          | |          | --- forwarder --- |          | |
               |__ first _|_|_ second _|                   |__ first _|_|_ second
               |          |                                |          |
               |__ first _|                                |__ first _|
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

class node():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class forwarder():
    def svc(self, lis: list):
        new_lis = lis.copy()
        new_lis.append("forwarder-to-first4")
        ff_send_out(new_lis, 3)
        lis.append("forwarded-to-any")
        return lis
    
class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        print(lis)

def run_test(use_subinterpreters = True):
    pipe = FFPipeline(use_subinterpreters)

    source_node = source()
    pipe.add_stage(source_node)

    first_a2a = FFAllToAll(use_subinterpreters)
    # build first stages
    first_lis = [node(f"first{i+1}") for i in range(4)]
    # build second stages
    second_lis = [node(f"second{i+1}") for i in range(3)]
    # add first stages
    first_a2a.add_firstset(first_lis)
    # add second stages
    first_a2a.add_secondset(second_lis)

    pipe.add_stage(first_a2a)

    fwdnode = forwarder()
    pipe.add_stage(fwdnode)

    second_a2a = FFAllToAll(use_subinterpreters)
    # build first stages
    first_lis = [node(f"first{i+1}") for i in range(4)]
    # build second stages
    second_lis = [sink(f"second{i+1}") for i in range(3)]
    # add first stages
    second_a2a.add_firstset(first_lis)
    # add second stages
    second_a2a.add_secondset(second_lis)

    pipe.add_stage(second_a2a)
    
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