# Just a pipeline with two stages
#
#   Stage1 -----> Stage2

from fastflow import ff_pipeline, ff_node, STOP, GO_ON

class Stage1(ff_node):
    def __init__(self):
        ff_node.__init__(self)
    
    def svc_init(self):
        self.counter = 0
        return 0

    def svc(self, args):
        value_to_next_stage = self.counter
        if self.counter > 10:
            return STOP # bug: return None is equivalent to return STOP
        
        self.counter = self.counter + 1
        print("Hi!")

        return value_to_next_stage
    
    def svc_end(self):
        print("first stage ended")

class Stage2(ff_node):
    def __init__(self):
        ff_node.__init__(self)

    def svc(self, args):
        print("Got", args, "from previous stage")

        return GO_ON
    
    def svc_end(self):
        print("second stage ended")

def main():
    # create a pipeline and the stage
    pipe = ff_pipeline() 
    stage1 = Stage1()
    stage2 = Stage2()
    pipe.add_stage(stage1) # add the first stage
    pipe.add_stage(stage2) # then add the second stage

    # blocking call: run the pipeline and wait for it to end
    if pipe.run_and_wait_end() < 0:
        raise("running pipeline") # throw exception if there is a failure
    
    # print stats
    print("DONE, time =", pipe.ffTime(), "(ms)")
    
if __name__ == "__main__":
    main()