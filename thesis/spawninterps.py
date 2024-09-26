from threading import Thread
import _xxsubinterpreters as subinterpreters
import time

def spawn_in_parallel():
    def spawn_sub():
        sid = subinterpreters.create(isolated=True)
        #subinterpreters.run_string(sid, "")
        #subinterpreters.destroy(sid)

    threads = []
    for _ in range(100):
        t = Thread(target=spawn_sub)
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

def bench_subinterpreters():
    def spawn_sub():
        for _ in range(100):
            sid = subinterpreters.create(isolated=True)
            subinterpreters.run_string(sid, "")
            subinterpreters.destroy(sid)

    
    t = Thread(target=spawn_sub)
    t.start()
    t.join()


if __name__ == "__main__":
    start = time.clock_gettime(time.CLOCK_MONOTONIC)

    spawn_in_parallel()

    end = time.clock_gettime(time.CLOCK_MONOTONIC)
    print("spawn_in_parallel", ((end - start)))