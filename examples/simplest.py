# python version of tests/simplest.cpp

from fastflow import ff_pipeline, ff_node, STOP, GO_ON

class Stage(ff_node):
    def __init__(self):
        ff_node.__init__(self)
        self.counter = 0
    
    def svc_init(self):
        print("Hello, I'm Stage");
        return 0

    def svc(self, *args):
        self.counter = self.counter + 1
        if self.counter > 10:
            return STOP
        
        print("Hi!")

        return GO_ON
    
    def svc_end(self):
        print("stage ended")

def main():
    pipe = ff_pipeline()
    pipe.add_stage(Stage())
    
    if pipe.run_and_wait_end() < 0:
        raise RuntimeError("running pipeline")
    
    print("DONE, pipe  time=", pipe.ffTime(), "(ms)")
    
if __name__ == "__main__":
    main()
    
