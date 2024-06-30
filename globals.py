import math, types
from string import ascii_letters, capwords

def MyClass():
    pass

def myfun():
    str = "a string"

import inspect
def filter_globals():
    glb = [[k,v] for k,v in globals().items() if not (k.startswith('__') and k.endswith('__'))]
    
    res = ""
    for [k, v] in glb:
        try:
            if inspect.ismodule(v):
                res += f"import {k}"
            elif inspect.isclass(v) or inspect.isfunction(v):
                res += inspect.getsource(v)
            #else:
            #    res += f"{k} = {v}"
            res += "\n"
        except:
            pass
    print(res)
    

if __name__ == "__main__":
    filter_globals()