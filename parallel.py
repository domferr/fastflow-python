from fastflow import ff_pipeline, STOP
import fastflow
import inspect

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

def fun1(*arg):
    print("[1] python function called with", arg)

    # busy_work
    i = 0
    while (i < 40000000):
        i = i + 1
    
    return "the number is 17"#[51, 23, 25, 67, 17]

def fun2(*arg):
    print("[2] python function called with", arg)

    # busy_work()
    i = 0
    while (i < 40000000):
        i = i + 1
    
    return 51

def main():
    # create a pipeline and the stage
    pipe = ff_pipeline() 
    stage1 = fastflow.ff_node(inspect.getsource(fun1), fun1.__name__)
    stage2 = fastflow.ff_node(inspect.getsource(fun2), fun2.__name__)
    pipe.add_stage(stage1) # add the first stage
    pipe.add_stage(stage2) # then add the second stage

    # blocking call: run the pipeline and wait for it to end
    if pipe.run_and_wait_end() < 0:
        raise RuntimeError("running pipeline") # throw exception if there is a failure
    
    # print stats
    print("DONE, time =", pipe.ffTime(), "(ms)")
    
if __name__ == "__main__":
    main()