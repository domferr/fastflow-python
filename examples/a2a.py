from fastflow_module import FFAllToAll
import argparse
import sys
import string

class DummyData:
    def __init__(self):
        self.data = [string.ascii_letters * int(1024 / len(string.ascii_letters))] # 1024 bytes

    def __str__(self):
        return f"{self.data}"
    
    __repr__ = __str__

class firststage():
    def __init__(self, n_tasks):
        self.n_tasks = n_tasks

    def svc(self, *args):
        print(f'[firststage] svc, remaining tasks = {self.n_tasks}')
        if self.n_tasks == 0:
            return None
        self.n_tasks -= 1

        return DummyData()

class secondstage():    
    def svc(self, *args):
        print('[secondstage] svc')

        return args

def build_a2a(first_stage_size, second_stage_size, use_processes = True, use_subinterpreters = False):
    a2a = FFAllToAll(use_subinterpreters)

    n_tasks = 64
    # build first stages
    first_lis = [firststage(n_tasks) for i in range(first_stage_size)]
    # build second stages
    second_lis = [secondstage() for i in range(second_stage_size)]
    
    # add first stages
    if use_processes:
        a2a.add_firstset_process(first_lis)
    else:
        a2a.add_firstset(first_lis)
    
    # add second stages
    if use_processes:
        a2a.add_secondset_process(second_lis)
    else:
        a2a.add_secondset(second_lis)

    return a2a

def run_a2a(first_stage_size, second_stage_size, use_processes = False, use_subinterpreters = False, blocking_mode = None, no_mapping = False):
    print(f"run a2a of {first_stage_size} first stages and {second_stage_size} second stages", file=sys.stderr)
    a2a = build_a2a(first_stage_size, second_stage_size, use_processes, use_subinterpreters)
    if blocking_mode is not None:
        a2a.blocking_mode(blocking_mode)
    if no_mapping:
        a2a.no_mapping()
    a2a.run_and_wait_end()
    return a2a.ffTime()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('-firststage', type=int, help='How many first stages', required=True)
    parser.add_argument('-secondstage', type=int, help='How many second stages', required=True)
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    args = parser.parse_args()

    if args.proc:
        res = run_a2a(args.firststage, args.secondstage, use_processes = True, use_subinterpreters = False)
        print(f"done in {res}ms")
    elif args.sub:
        res = run_a2a(args.firststage, args.secondstage, use_processes = False, use_subinterpreters = True)
        print(f"done in {res}ms")