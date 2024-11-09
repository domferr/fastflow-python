import os
import sys
from concurrent.futures import _base
import threading # for locking mechanisms
from . import FFFarm, EOS

class _item(object):
    def __init__(self, future_id, fn, args, kwargs):
        self.future_id = future_id
        self.fn = fn
        self.args = args
        self.kwargs = kwargs

class _worker():
    def __init__(self, initializer, initargs):
        self._initializer = initializer
        self._initargs = initargs
    
    def svc_init(self):
        if self._initializer:
            self._initializer(self._initargs)

    def svc(self, item: _item):
        res = item.fn(*item.args, **item.kwargs)
        return item.future_id, res

class FastFlowExecutor(_base.Executor):
    def __init__(self, max_workers=None, use_subinterpreters=False,
                 initializer=None, initargs=()):
        """Initializes a new FastFlowExecutor instance.

        Args:
            max_workers: The maximum number of nodes that can be used to
                execute the given calls. If None or not given then as many
                worker processes will be created as the machine has processors.
            use_subinterpreters: Whether to use multiprocessing or subinterpreters API
            initializer: A callable used to initialize worker processes.
            initargs: A tuple of arguments to pass to the initializer.
        """

        self._max_workers = max_workers
        if self._max_workers is None:
            self._max_workers = os.process_cpu_count() or 1
        elif self._max_workers <= 0:
            raise ValueError("max_workers must be greater than 0")

        if use_subinterpreters and sys.version_info[1] < 12:
            raise ValueError("Subinterpreters are supported from Python 3.12+")

        if initializer is not None and not callable(initializer):
            raise TypeError("initializer must be a callable")

        self._shutdown = False
        self._last_id = 1
        self._pending_futures = {}
        self._futures_lock = threading.Lock()
        self._farm = FFFarm(use_subinterpreters)
        self._farm.no_mapping()
        self._farm.blocking_mode(True)
        self._farm.set_scheduling_ondemand(True)
        self._farm.add_workers([_worker(initializer, initargs) for _ in range(self._max_workers)])
        self._farm.add_collector(_collector(self), use_main_thread=True)
        self._farm.add_emitter(_emitter(self), use_main_thread=True)
        self._farm.run()

    def submit(self, fn, /, *args, **kwargs):
        if self._shutdown:
            raise RuntimeError('cannot schedule new futures after shutdown')

        f = _base.Future()
        with self._futures_lock:
            future_id = self._last_id
            success = self._farm.submit(_item(future_id, fn, args, kwargs))
            if not success:
                raise RuntimeError('failed to submit')
            self._last_id = self._last_id + 1
            self._pending_futures[future_id] = f
        return f
    submit.__doc__ = _base.Executor.submit.__doc__

    def shutdown(self, wait=True, *, cancel_futures=False):
        if self._shutdown:
            return
        self._shutdown = True
        self._farm.submit(EOS)
        if cancel_futures:
            with self._futures_lock:
                for future in self._pending_futures:
                    future.cancel()
        if wait:
            self._farm.wait()
            

    shutdown.__doc__ = _base.Executor.shutdown.__doc__

class _emitter():
    def __init__(self, executor: FastFlowExecutor):
        self._executor = executor

    def svc(self, item: _item):
        with self._executor._futures_lock:
            future: _base.Future = self._executor._pending_futures[item.future_id]
            future.set_running_or_notify_cancel()
            if not future.cancelled():
                return item

class _collector():
    def __init__(self, executor: FastFlowExecutor):
        self._executor = executor

    def svc(self, future_id, result):
        with self._executor._futures_lock:
            future: _base.Future = self._executor._pending_futures[future_id]
            future.set_result(result)