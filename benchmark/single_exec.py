import busy_wait
import time

start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
busy_wait.wait(5)
end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

elapsed_ns = end - start
print(elapsed_ns / 1_000_000, "ms")