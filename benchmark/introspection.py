from PIL import Image, ImageFilter
import pickle

glb = [[k,v] for k,v in globals().items() if not (k.startswith('__') and k.endswith('__'))]

import inspect
__ff_res = ""
for [k, v] in glb:
    try:
        if inspect.ismodule(v):
            print(v.__package__)
            pkg = v.__package__
            if pkg:
                __ff_res += f"from {pkg} import {k}"
            else: # pkg is empty is the module and the package are the same
                __ff_res += f"import {k}"
        elif inspect.isclass(v) or inspect.isfunction(v):
            __ff_res += inspect.getsource(v)
        #else:
        #    __ff_res += f"{k} = {v}"
        __ff_res += "\n"
    except:
        pass

print(__ff_res)