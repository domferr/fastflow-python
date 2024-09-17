from fastflow_module import FFAllToAll, FFPipeline, EOS
import sys

"""
    source1 - stage11 - stage21 _   _ sink1
                                 | |
    source2 - stage12 - stage22 _|_|_ sink2
                                 | |
    source3 - stage13 - stage23 _|_|_ sink3
                                 |
    source4 - stage14 - stage24 _|
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

class pipestage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class lastpipestage():
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
    a2a = FFAllToAll(use_subinterpreters)
    first_stage_size = 4
    second_stage_size = 3
    # build first stages
    first_lis = []
    for i in range(first_stage_size):
        pipe = FFPipeline(use_subinterpreters)
        sourcenode = source(f'source{i+1}')
        st1 = pipestage(f'st{i+1}-1')
        st2 = lastpipestage(f'st{i+1}-2')
        pipe.add_stage(sourcenode)
        pipe.add_stage(st1)
        pipe.add_stage(st2)
        first_lis.append(pipe)
    
    # add first stages
    a2a.add_firstset(first_lis)

    # build second stages
    second_lis = [sink(f'sink{i+1}') for i in range(second_stage_size)]
    # add second stages
    a2a.add_secondset(second_lis)

    a2a.run_and_wait_end()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)