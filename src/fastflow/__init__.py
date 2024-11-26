from ._fastflow import *
import threading
import queue

messaging = None

def ff_send_out(data, index=0):
    global messaging
    if messaging is None:
        raise RuntimeError("Operation not supported")
    return messaging.ff_send_out(data, index)

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

def process_body(node, readfd, writefd, isMultiOutput):
    global messaging
    messaging = Messaging(readfd, writefd, isMultiOutput)

    loop = True
    if hasattr(node, 'svc_init'):
        res = getattr(node, 'svc_init')()
        if res is None:
            res = 0
        messaging.send_out_int(res)
        loop = res == 0

    while loop:
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

def _ff_spawner(_spawner_list):
    for proc in _spawner_list:
        proc.start()

def _before_building_block_run():
    global _spawner_list, _spawner_thread
    _spawner_list = list()

    # create the ff_spawner thread
    _spawner_thread = threading.Thread(target=_ff_spawner, args=(_spawner_list, ))

def _after_building_block_run():
    global _spawner_thread
    _spawner_thread.start()

def _after_building_block_wait():
    global _spawner_thread, _spawner_list
    _spawner_thread.join()
    _spawner_thread = None
    _spawner_list = None

def _start_fastflow_process(node, readfd, writefd, isMultiOutput, start = False):
    proc = multiprocessing.get_context().Process(
        target=process_body, 
        args=(node, readfd, writefd, isMultiOutput)
    )

    if start:
        proc.start()
    else:
        global _spawner_list
        _spawner_list.append(proc)

    return proc