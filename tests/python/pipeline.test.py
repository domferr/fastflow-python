from fastflow_module import FFPipeline

class source():
    def __init__(self):
        self.counter = 1

    def svc(self):
        if self.counter == 6:
            return None
        self.counter += 1

        return list(["source"])

class stage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        lis.append("sink")
        print(lis)
        return 0

def run_test(use_subinterpreters = True, use_processes = False):
    pipe = FFPipeline(use_subinterpreters)
    sourcenode = source()
    st1 = stage('st1')
    st2 = stage('st2')
    sinknode = sink()
    if use_processes:
        pipe.add_stage_process(sourcenode)
    else:
        pipe.add_stage(sourcenode)
    if use_processes:
        pipe.add_stage_process(st1)
    else:
        pipe.add_stage(st1)
    if use_processes:
        pipe.add_stage_process(st2)
    else:
        pipe.add_stage(st2)
    if use_processes:
        pipe.add_stage_process(sinknode)
    else:
        pipe.add_stage(sinknode)
    pipe.run_and_wait_end()

if __name__ == "__main__":
    print("Subinterpreters")
    run_test(True, False)
    print()
    print("Processes")
    run_test(False, True)