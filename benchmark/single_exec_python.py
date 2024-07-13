import time
import math

def busy_work():
    thousands = 245000 # 1000ms repara
    hundreds = 56950 # 100ms repara
    tens = 15370 # 10ms repara
    
    math.factorial(tens)

def loop_busy_work():
    thousands = 24000000 # 1000ms repara
    hundreds = 2570000 # 100ms repara
    tens = 190000 # 10ms repara

    i = 0
    while (i < thousands):
        i = i + 1

start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
busy_work()
end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

elapsed_ns = end - start
print(elapsed_ns / 1_000_000, "ms")