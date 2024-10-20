from fastflow import FFPipeline, EOS
import sys

class stage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        lis.append("sink")
        return lis

def run_test(use_subinterpreters, use_main_thread = False):
    pipe = FFPipeline(use_subinterpreters)
    st1 = stage('st1')
    st2 = stage('st2')
    st3 = stage('st3')
    sinknode = sink()
    pipe.add_stage(st1, use_main_thread=use_main_thread)
    pipe.add_stage(st2, use_main_thread=use_main_thread)
    pipe.add_stage(st3, use_main_thread=use_main_thread)
    pipe.add_stage(sinknode, use_main_thread=use_main_thread)
    pipe.run()
    for i in range(8):
        pipe.submit([f"data{i}"])
    pipe.submit(EOS)
    pipe.wait()

    lis = pipe.collect_next()
    while lis is not None:
        print(lis)
        lis = pipe.collect_next()

if __name__ == "__main__":
    if sys.version_info[1] >= 12:
        print("Subinterpreters")
        run_test(use_subinterpreters = True)
    else:
        print("Skip subinterpreters test because python version is < 3.12")
    print()
    print("Processes")
    run_test(use_subinterpreters = False)