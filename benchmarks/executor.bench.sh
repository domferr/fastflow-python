#!/bin/bash

# default values
TASKS=2
MS=1000
BYTES=100
STRATEGY=-ffproc #one between -ffproc | -ffsub | -threadpool | -processpool
PYTHON=python3
# parse script args
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --tasks) TASKS="$2"; shift ;;
        --ms) MS="$2"; shift ;;
        --bytes) BYTES="$2"; shift ;;
        --strategy) STRATEGY="$2"; shift ;;
        --python) PYTHON="$2"; shift ;;
        --help)  echo "USAGE: ./executor.bench.sh --tasks 4 --ms 2000 --bytes 500 --strategy (-ffproc | -ffsub | -threadpool | -processpool)"; exit 0 ;;
        *) echo "Invalid arg: $1" >&2; exit 1 ;;
    esac
    shift
done

printf "workers, tasks, ms, bytes, strategy, elapsed time (ms)\n" $TASKS $MS $BYTES $STRATEGY
for worker in 1 2 4 8 16 24 32 48 61 62 63 64;
do
    printf "%d, %d, %d, %d, %s, " $worker $TASKS $MS $BYTES $STRATEGY
    $PYTHON benchmarks/executor.bench.py -tasks $TASKS -workers $worker -ms $MS -bytes $BYTES $STRATEGY
done;