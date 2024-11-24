from ._fastflow import *
from .executor import FastFlowExecutor
from .executorfarm import FastFlowFarmExecutor
import multiprocessing

__all__ = [
    "FFPipeline",
    "FFFarm",
    "FFAllToAll",
    "EOS",
    "GO_ON",
    "ff_send_out",
    "FastFlowExecutor",
    "FastFlowFarmExecutor"
]

def ff_send_out(data, index=0):
    global messaging
    return messaging.ff_send_out(data, index)

def process_body(node, readfd, writefd, isMultiOutput):
    global messaging
    messaging = Messaging(readfd, writefd, isMultiOutput)

    while True:
        input = messaging.get_input()
        if input['eol']:
            break
        func = getattr(node, input['fun_name'])
        if isinstance(input['data'], tuple):
            output = func(*input['data'])
        else:
            output = func(input['data'])
        messaging.send_out(output)
    messaging.closefds()

def spawn_fastflow_process(node, readfd, writefd, isMultiOutput):
    return multiprocessing.Process(
        target=process_body, 
        args=(node, readfd, writefd, isMultiOutput)
    )