import pickle
import inspect
import fastflow
from fastflow import ff_pipeline, STOP

class Stage1():    
    def svc(self, *args):
        for count in range(1, 4):
            print("[1] Stage1 svc busy work")

            # busy_work
            i = 0
            while (i < 40000000):
                i = i + 1

            self.ff_send_out(count)
            
        return STOP

class Stage2():
    def svc(self, args):
        print("[2] Stage2 svc called with", args)

        # busy_work
        i = 0
        while (i < 40000000):
            i = i + 1
            
        return 17

def main():
    # create a pipeline and the stage
    original = Stage1()
    stage1 = fastflow.to_ff_node(f"{pickle.dumps(original)}", inspect.getsource(original.__class__))
    stage2 = fastflow.to_ff_node(f"{pickle.dumps(Stage2())}", inspect.getsource(Stage2().__class__))

    pipe = ff_pipeline()
    pipe.add_stage(stage1)
    pipe.add_stage(stage2)

    # blocking call: run the pipeline and wait for it to end
    if pipe.run_and_wait_end() < 0:
        raise RuntimeError("running pipeline") # throw exception if there is a failure
    
    # print stats
    print("DONE, pipeline time =", pipe.ffTime(), "(ms)")
    
if __name__ == "__main__":
    main()