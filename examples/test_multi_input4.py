# Python version of tests/test_multi_input4.cpp
#
#   Stage1 -----> Stage2 -----> Stage3
#                  ^               |
#                   \--------------   

from fastflow import ff_pipeline, ff_node, ff_minode, STOP, GO_ON, EOS

NUMTASKS = 1000

class Stage1(ff_node):
    def __init__(self):
        ff_node.__init__(self)
    
    def svc(self, *args):
        for i in range(1, NUMTASKS+1):
            self.ff_send_out(i)

        return STOP

class Stage2(ff_minode):
    def __init__(self):
        ff_minode.__init__(self)
    
    def svc(self, arg):
        print(f"Stage2 got task from {self.get_channel_id()}")
        if self.fromInput():
            return arg

        return GO_ON

    def eosnotify(self, *args):
        self.ff_send_out(EOS)

class Stage3(ff_node):
    def __init__(self):
        ff_node.__init__(self)
    
    def svc(self, arg):
        print(f"Stage3: got {arg}, sending it back")

        return arg

def main():
    s1 = Stage1()
    s2 = Stage2()
    s3 = Stage3()

    pipe2 = ff_pipeline()
    pipe2.add_stage(s2)
    pipe2.add_stage(s3)

    if pipe2.wrap_around() < 0:
        raise RuntimeError("wrap_around")

    pipe = ff_pipeline()
    pipe.add_stage(s1)
    pipe.add_stage(pipe2)
    
    if pipe.run_and_wait_end() < 0:
        raise RuntimeError("running pipe")
    
    print("DONE")
    
if __name__ == "__main__":
    main()

