from fastflow import FFAllToAll, EOS
import sys

"""
     stage _   _ sink
            | |
     stage _|_|_ sink
            | |
     stage _|_|_ sink
            |
     stage _|
"""

class stage():
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
    first_lis = [stage(i+1) for i in range(first_stage_size)]
    # build second stages
    second_lis = [stage(f"sink{i+1}") for i in range(second_stage_size)]
    # add first stages
    a2a.add_firstset(first_lis)
    # add second stages
    a2a.add_secondset(second_lis)
    a2a.run()
    for i in range(8):
        a2a.submit([f"data{i}"])
    a2a.submit(EOS)
    a2a.wait()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)