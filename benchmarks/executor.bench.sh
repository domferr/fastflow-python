#!/bin/bash

# default values
TASKS=2
TASKINFO=1000 #a number if synthetic benchmark (indicating milliseconds of one task), -numpy otherwise
BYTES=100
STRATEGY=-ffproc #one between -ffproc | -fffarm | -ffsub | -threadpool | -processpool
PYTHON=python3
RUNS=1
# parse script args
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --tasks) TASKS="$2"; shift ;;
        --ms) TASKINFO="-ms $2"; shift ;;
        --bytes) BYTES="$2"; shift ;;
        --strategy) STRATEGY="$2"; shift ;;
        --python) PYTHON="$2"; shift ;;
        --taskinfo) TASKINFO="$2"; shift ;;
        --runs) RUNS="$2"; shift ;;
        --help)  echo "USAGE: ./executor.bench.sh --tasks 4 --ms 2000 --bytes 500 --strategy (-ffproc | -fffarm | -ffsub | -threadpool | -processpool)"; exit 0 ;;
        *) echo "Invalid arg: $1" >&2; exit 1 ;;
    esac
    shift
done

printf "workers, tasks, ms, bytes, strategy, runs, "
for (( i=1; i<=$RUNS; i++ )) ; {
    printf "run$i"
    if (( $i <= $RUNS - 1 )); then
        printf ", "
    fi
}
printf "\n"

for worker in 1 4 16 32 48 54 60 62 64;
do
    printf "%d, %d, %s, %s, %s, %s, " $worker $TASKS "$TASKINFO" $BYTES $STRATEGY $RUNS
    for (( i=0; i<$RUNS; i++ )) ; {
        $PYTHON benchmarks/executor.bench.py -tasks $TASKS -workers $worker $TASKINFO -bytes $BYTES $STRATEGY
        if (( $i < $RUNS - 1 )); then
            printf ", "
        fi
    }
    printf "\n"
done;