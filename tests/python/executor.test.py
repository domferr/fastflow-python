import os
import sys
import concurrent.futures
from concurrent.futures import _base
from fastflow import FFFarm, EOS

class _worker():
    def __init__(self, initializer, initargs):
        self._initializer = initializer
        self._initargs = initargs
    
    def svc_init(self):
        if self._initializer:
            self._initializer(self._initargs)

    def svc(self, future_id, fn):
        res = fn()
        return future_id, res

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
        self._farm = FFFarm(use_subinterpreters)
        self._farm.no_mapping()
        self._farm.blocking_mode(True)
        self._farm.add_workers([_worker(initializer, initargs) for _ in range(self._max_workers)])
        self._farm.add_collector(_collector(self), use_main_thread=True)
        self._farm.add_emitter(_emitter(self), use_main_thread=True)
        self._farm.run()

    def submit(self, fn, /, *args, **kwargs):
        if self._shutdown:
            raise RuntimeError('cannot schedule new futures after shutdown')
        
        f = _base.Future()
        f.cancel()
        future_id = self._last_id
        success = self._farm.submit((future_id, fn))
        if not success:
            raise RuntimeError('failed to submit')
        self._last_id = self._last_id + 1
        self._pending_futures[future_id] = f
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
        self._farm.submit(EOS)
        if cancel_futures:
            for future in self._pending_futures:
                future.cancel()
            #self._pending_futures = []
        if wait:
            self._farm.wait()
            

    shutdown.__doc__ = _base.Executor.shutdown.__doc__

class _emitter():
    def __init__(self, executor: FastFlowExecutor):
        self._executor = executor

    def svc(self, future_id, fn):
        future: _base.Future = self._executor._pending_futures[future_id]
        future.set_running_or_notify_cancel()
        if future.cancelled():
            return
        return future_id, fn

class _collector():
    def __init__(self, executor: FastFlowExecutor):
        self._executor = executor

    def svc(self, future_id, result):
        future: _base.Future = self._executor._pending_futures[future_id]
        future.set_result(result)
        return

def example():
    print("hey!")
    for _ in range(2**24):
        pass

if __name__ == "__main__":
    max_workers = 2
    tasks = 4
    """exe = FastFlowExecutor(max_workers=max_workers)
    result = [exe.submit(example) for _ in range(tasks)]
    concurrent.futures.wait(result)
    time.sleep(10)"""
    with FastFlowExecutor(max_workers=max_workers) as exe:
        # issue tasks
        result = [exe.submit(example) for _ in range(tasks)]
        # wait for the tasks to complete
        concurrent.futures.wait(result)

    """farm = FFFarm()
    #farm.no_mapping()
    #farm.blocking_mode(True)
    farm.add_workers([_worker(None, None) for _ in range(max_workers)])
    farm.add_emitter(_emitter2(), use_main_thread=True)
    farm.add_collector(_collector2(), use_main_thread=True)
    farm.run()
    for i in range(tasks):
        submit(i, farm, example)
    #result = [submit(i, farm, example) for i in range(tasks)]
    #concurrent.futures.wait(result)
    time.sleep(6)"""