from parallel import node
import inspect
from math import exp

def busy_work():
    i = 0
    while (i < 40000000):
        i = i + 1

def fun1(arg):
    print("[1] python function called with", arg)

    # busy_work
    i = 0
    while (i < 40000000):
        i = i + 1
    
    return 17

def fun2(arg):
    print("[2] python function called with", arg)

    #busy_work()
    i = 0
    while (i < 40000000):
        i = i + 1
    
    return 51

class pynode:
    def __init__(self):
        self.node = node()

    def run(self, fun):
        #print(fun.__globals__)
        
        #print([inspect.getsource(f) for f in globals().values() if type(f) == type(fun)])
        self.node.run(inspect.getsource(fun), fun.__name__)
    
    def join(self):
        self.node.join()

node1 = pynode()
node1.run(fun1)

node2 = pynode()
node2.run(fun2)

node3 = pynode()
node3.run(fun2)

node1.join()
node2.join()
node3.join()