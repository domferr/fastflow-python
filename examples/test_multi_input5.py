#      Stage0 -----> Stage1 -----> Stage2 -----> Stage3
#      ^  ^ ^             |          |              | 
#       \  \ \------------           |              |
#        \  \-------------------------              |
#         \-----------------------------------------    

from fastflow import ff_pipeline, ff_node, ff_minode, ff_monode, GO_ON, EOS

NUMTASKS = 10

class Stage0(ff_minode):
    def __init__(self):
        ff_minode.__init__(self)

    def svc_init(self):
        self.counter = 0
        return 0

    def svc(self, task):
        if task == ():
            for i in range(1, NUMTASKS + 1):
                val = i
                self.ff_send_out(val)
            return GO_ON
        
        print(f"Stage0 got back {task} from {self.get_channel_id()} (counter={self.counter + 1})")
        self.counter += 1
        if self.counter == NUMTASKS:
            return EOS
        return GO_ON

class Stage1(ff_monode):
    def __init__(self):
        ff_monode.__init__(self)

    def svc(self, task):
        if task % 2 == 1: # sends odd tasks back
            print(f"Stage1 sending back={task}")
            self.ff_send_out(task) 
        else:
            print(f"Stage1 sending forward={task}")
            self.ff_send_out_to(task, 1) # sends it forward
        return GO_ON

    def eosnotify(self, *args):
        self.ff_send_out(EOS)

class Stage2(ff_monode):
    def __init__(self):
        ff_monode.__init__(self)

    def svc(self, task):
        if task <= (NUMTASKS / 2):
            print(f"Stage2 sending back={task}")
            self.ff_send_out(task) # sends even tasks less than <NUMTASKS / 2> back
        else:
            print(f"Stage2 sending forward={task}")
            self.ff_send_out_to(task, 1)
        return GO_ON

    def eosnotify(self, *args):
        self.ff_send_out(EOS)

class Stage3(ff_node):
    def __init__(self):
        ff_node.__init__(self)

    def svc(self, task):
        print(f"Stage3 got task {task}")
        if task <= NUMTASKS / 2 or task % 2 == 1:
            raise RuntimeError("received a wrong task")
        
        return task

def main():
    s0 = Stage0()
    s1 = Stage1()
    s2 = Stage2()
    s3 = Stage3()

    """pipe = ff_pipeline()
    pipe.add_stage(s0)
    pipe.add_stage(s1)
    pipe.wrap_around()
    pipe.add_stage(s2)
    pipe.wrap_around()
    pipe.add_stage(s3)
    pipe.wrap_around()"""

    pipe1 = ff_pipeline()
    pipe1.add_stage(s0)
    pipe1.add_stage(s1)
    pipe1.wrap_around()
    
    pipe2 = ff_pipeline()
    pipe2.add_stage(pipe1)
    pipe2.add_stage(s2)
    pipe2.wrap_around()
 
    pipe = ff_pipeline()
    pipe.add_stage(pipe2)
    pipe.add_stage(s3)
    pipe.wrap_around();

    if pipe.run_and_wait_end() < 0:
        raise RuntimeError("running pipe")

    print("DONE")

if __name__ == "__main__":
    main()
