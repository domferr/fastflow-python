import os
import sys
from concurrent.futures import _base
from collections import deque
import threading # for locking mechanisms
from . import FFFarm, EOS, GO_ON

MAGIC_EOS_VALUE = 17

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

class FastFlowFarmExecutor(_base.Executor):
    def __init__(self, max_workers=None, use_subinterpreters=False,
                 initializer=None, initargs=()):
        """Initializes a new FastFlowFarmExecutor instance.

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
        self._pending_items = deque()
        self._lock = threading.Lock()
        self._not_empty = threading.Condition(self._lock)
        self._farm = FFFarm(use_subinterpreters)
        self._farm.no_mapping()
        self._farm.blocking_mode(True)
        self._farm.set_scheduling_ondemand(True)
        self._farm.add_workers([_worker(initializer, initargs) for _ in range(self._max_workers)])
        self._farm.add_collector(_collector(self), use_main_thread=True)
        self._farm.add_emitter(_emitterfarm(self), use_main_thread=True)
        self._farm.run(False)

    def submit(self, fn, /, *args, **kwargs):
        if self._shutdown:
            raise RuntimeError('cannot schedule new futures after shutdown')

        f = _base.Future()
        with self._lock:
            future_id = self._last_id
            self._pending_items.appendleft(_item(future_id, fn, args, kwargs))
            self._pending_futures[future_id] = f
            self._last_id = self._last_id + 1
            self._not_empty.notify()
        return f
    submit.__doc__ = _base.Executor.submit.__doc__

    def map(self, fn, *iterables, timeout=None, chunksize=1):
        """Returns an iterator equivalent to map(fn, iter).

        Args:
            fn: A callable that will take as many arguments as there are
                passed iterables.
            timeout: The maximum number of seconds to wait. If None, then there
                is no limit on the wait time.
            chunksize: If greater than one, the iterables will be chopped into
                chunks of size chunksize and submitted to the process pool.
                If set to one, the items in the list will be sent one at a time.

        Returns:
            An iterator equivalent to: map(func, *iterables) but the calls may
            be evaluated out-of-order.

        Raises:
            TimeoutError: If the entire result iterator could not be generated
                before the given timeout.
            Exception: If fn(*args) raises for any values.
        """
        raise BaseException("Not implemented yet")
        if chunksize < 1:
            raise ValueError("chunksize must be >= 1.")

        results = super().map(partial(_process_chunk, fn),
                              itertools.batched(zip(*iterables), chunksize),
                              timeout=timeout)
        return _chain_from_iterable_of_lists(results)

    def shutdown(self, wait=True, *, cancel_futures=False):
        if self._shutdown:
            return
        self._shutdown = True
        with self._lock:
            if cancel_futures:
                self._pending_items.clear()
                for future in self._pending_futures:
                    future.cancel()
            self._pending_items.appendleft(MAGIC_EOS_VALUE)
            self._not_empty.notify()
        if wait:
            self._farm.wait()
            

    shutdown.__doc__ = _base.Executor.shutdown.__doc__

class _emitterfarm():
    def __init__(self, executor: FastFlowFarmExecutor):
        self._executor = executor

    def svc(self, *args):
        with self._executor._lock:
            # wait until there are pending items
            while not len(self._executor._pending_items):
                self._executor._not_empty.wait()
            # get the first pending item (FIFO order)
            item: _item = self._executor._pending_items.pop()
            # if the pending item is the EOS flag, return EOS
            if item is MAGIC_EOS_VALUE:
                return EOS
            # set the item's future to running and schedule it
            future: _base.Future = self._executor._pending_futures[item.future_id]
            future.set_running_or_notify_cancel()
            if not future.cancelled():
                return item

class _collector():
    def __init__(self, executor: FastFlowFarmExecutor):
        self._executor = executor

    def svc(self, future_id, result):
        with self._executor._lock:
            future: _base.Future = self._executor._pending_futures[future_id]
            future.set_result(result)