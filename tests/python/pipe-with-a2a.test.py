from fastflow import FFAllToAll, FFPipeline, EOS
import sys

"""
         _ one --- two _
        |               |
source__|_ one --- two -|-- sink
        |               |
        |_ one --- two _|

"""

class source():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1

        return list([self.id])

class a2astage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis
    
class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)
        print(lis)

def run_test(use_subinterpreters = True):
    pipe = FFPipeline(use_subinterpreters)

    source_a2a = FFAllToAll(use_subinterpreters)
    second_stage_size = 3
    # build first stages
    source_first_lis = [source("source")]
    # add first stages
    source_a2a.add_firstset(source_first_lis)

    # build second stages
    second_lis = [a2astage(f'one{i+1}') for i in range(second_stage_size)]
    # add second stages
    source_a2a.add_secondset(second_lis)

    pipe.add_stage(source_a2a)

    a2a = FFAllToAll(use_subinterpreters)
    first_stage_size = second_stage_size
    # build first stages
    first_lis = [a2astage(f'two{i+1}') for i in range(first_stage_size)]
    # add first stages
    a2a.add_firstset(first_lis)

    # build second stages
    second_lis = [sink("sink")]
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