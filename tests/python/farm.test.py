from fastflow_module import FFFarm

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter == 12:
            return None
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
        return 0

def run_test(use_subinterpreters = True, use_processes = False):
    farm = FFFarm(use_subinterpreters)
    sourcenode = source()
    sinknode = sink()
    w_lis = []
    for i in range(6):
        w = worker(f"{i+1}")
        w_lis.append(w)
    if use_processes:
        farm.add_emitter_process(sourcenode)
    else:
        farm.add_emitter(sourcenode)
    if use_processes:
        farm.add_workers_process(w_lis)
    else:
        farm.add_workers(w_lis)
    if use_processes:
        farm.add_collector_process(sinknode)
    else:
        farm.add_collector(sinknode)
    farm.run_and_wait_end()

if __name__ == "__main__":
    print("Subinterpreters")
    run_test(True, False)
    print()
    print("Processes")
    run_test(False, True)