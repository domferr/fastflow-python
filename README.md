# FastFlow Python API

Unlock the full potential of parallel computing in **Python** with FastFlow, a powerful C++ library now available in Python, that brings **high-performance, scalable parallelism right to your fingertips**.

- 🤩 Implement advanced parallel patterns and building blocks, like pipelines, farms, and all-to-all, with ease.
- 🚀🚀 Experience lightning-fast parallel execution with zero boilerplate code.
- 💡 It overcomes Python’s Global Interpreter Lock (GIL) limitations for you. You can leverage on multiple processes or subinterpreters.

Whether you’re processing massive datasets, building scalable applications, or designing high-throughput systems, FastFlow delivers the speed and efficiency you've been looking for.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
  - [Farm](#farm)
  - [Pipeline](#pipeline)
  - [All-to-All](#all-to-all)
    - [Scheduling Policies](#scheduling-policies)
  - [Customize initialization and ending phases](#customize-initialization-and-ending-phases)
  - [Sending Out Multiple Data or to Specific Nodes](#sending-out-multiple-data-or-to-specific-nodes)
  - [Composing Building Blocks](#composing-building-blocks)
    - [All-to-All Inside Pipeline](#all-to-all-inside-pipeline)
    - [Pipeline Inside All-to-All](#pipeline-inside-all-to-all)
  - [Blocking mode](#blocking-mode)
  - [Mapping and unmapping to cores](#mapping-and-unmapping-to-cores)
- [Contributing](#contributing)

## Installation

To install **fastflow**, ensure you have the following dependencies: `python3`, `python3-venv` and `python3-dev`. For example on Ubuntu you can install them using `apt install`.

1. From the root directory, create virtual environment via `python3 -m venv .venv`
2. From the root directory, activate the virtual environment by running `source .venv/bin/activate`
3. Build and install by running `make` from the root directory

## Usage

<img src="https://github.com/user-attachments/assets/f94620dc-3133-4002-9238-1416a602f10c" align="right" height="92"/>

### Farm

The Farm pattern consists of an emitter, multiple workers, and a collector. It distributes tasks from the emitter to workers and collects the results in the collector. Both emitter and collector are optional.

```py
from fastflow import FFFarm, EOS

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter == 12:
            return EOS
        self.counter += 1

        return list(['source'])

class worker():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        print(lis)


# create a farm, pass True as argument if you want to use subinterpreters
farm = FFFarm()
sourcenode = source()
sinknode = sink()
w_lis = []
for i in range(3):
    w = worker(f"worker{i+1}")
    w_lis.append(w)
farm.add_emitter(sourcenode)
farm.add_workers(w_lis)
farm.add_collector(sinknode)
# finally run the farm. Blocking call: will resume when the farm ends
farm.run_and_wait_end()
```
<img src="https://github.com/user-attachments/assets/65bd9fa0-4c01-47e9-afa4-82209352157d" align="right" height="42"/>

### Pipeline

The Pipeline pattern consists of multiple stages that process tasks in a linear sequence, where the output of one stage becomes the input of the next.

```py
from fastflow import FFPipeline, EOS
import sys

class source():
    def __init__(self):
        self.counter = 1

    def svc(self):
        if self.counter > 5:
            return EOS
        self.counter += 1

        return list(["source"])

class stage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis

class sink():    
    def svc(self, lis: list):
        lis.append("sink")
        print(lis)

# create a pipeline, pass True if you want to use subinterpreters
pipe = FFPipeline(True)
# create the first stage
sourcenode = source()
# create the second stage
st1 = stage('st1')
# create last stage
sinknode = sink()
# add all the stages
pipe.add_stage(sourcenode)
pipe.add_stage(st1)
pipe.add_stage(sinknode)
# finally run the pipeline. Blocking call: will resume when the pipeline ends
pipe.run_and_wait_end()
# print how many milliseconds the pipeline took
print(f"pipeline done in {farm.ffTime()}ms")
```

<img src="https://github.com/user-attachments/assets/cc074e33-d059-4edd-bef8-33a46d50ea9c" align="right" height="92"/>

### All-to-All

The All-to-All pattern connects multiple nodes to the left to other multiple nodes to the right, where every node to the left can send data to every node to the right.

```py
from fastflow import FFAllToAll, EOS

class leftnode():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return list([self.id])

class rightnode():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(f"rightnode{self.id}")
        print(lis)

# All-to-All with 4 nodes to the left and 3 nodes to the right
a2a = FFAllToAll()

# Create and add left nodes
a2a.add_firstset([leftnode(i+1) for i in range(4)])

# Create and add right nodes
a2a.add_secondset([rightnode(i+1) for i in range(3)])

# Run All-to-All
a2a.run_and_wait_end()
```

#### Scheduling Policies
In the All-to-All building block, two different scheduling policies can be used to distribute tasks from the left nodes to the right nodes:
- **Round Robin** (default): This policy distributes tasks evenly across the available right nodes in a cyclic manner. Each task is assigned to the next available right node in turn, regardless of the task's nature or the load on the right nodes. This method is efficient when tasks are uniform in size and processing time.
- **On-Demand**: In this policy, tasks are only sent to a right node when that node is ready to receive more tasks. It dynamically adapts to the load of each right node, making it more suited for scenarios where tasks have varying sizes or processing times. To enable on-demand scheduling, you can use the `ondemand=True` option in the `add_firstset()` method.

The following is an example of On-Demand scheduling:

```py
from fastflow import FFAllToAll, EOS

class source():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [f"data from source {self.id}"]

class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(f"processed by sink {self.id}")
        print(lis)

# All-to-All with on-demand scheduling
a2a = FFAllToAll()

# First stage (sources)
first_lis = [source(i+1) for i in range(3)]

# Second stage (sinks)
second_lis = [sink(i+1) for i in range(2)]

# Add stages using on-demand scheduling
a2a.add_firstset(first_lis, ondemand=True)
a2a.add_secondset(second_lis)

# Run All-to-All
a2a.run_and_wait_end()
```

### Customize initialization and ending phases

You can specify `svc_init` and `svc_end` for Initialization and Cleanup purposes. This example shows how to use `svc_init` for initialization and `svc_end` for finalization logic in stages.

```py
from fastflow import FFAllToAll, EOS

class source():
    def __init__(self, id):
        self.id = id
        self.counter = 1

    def svc_init(self):
        print(f"Source {self.id} initialized")

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [self.id]

    def svc_end(self):
        print(f"Source {self.id} finished")

class sink():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f"Sink {self.id} initialized")

    def svc(self, lis: list):
        lis.append(f"sink{self.id}")
        print(lis)

    def svc_end(self):
        print(f"Sink {self.id} finished")

# All-to-All setup
a2a = FFAllToAll()
first_stage_size = 3
second_stage_size = 3

# Create and add first stages (sources)
first_lis = [source(i+1) for i in range(first_stage_size)]
a2a.add_firstset(first_lis)

# Create and add second stages (sinks)
second_lis = [sink(i+1) for i in range(second_stage_size)]
a2a.add_secondset(second_lis)

# Run All-to-All
a2a.run_and_wait_end()
```

### Sending Out Multiple Data or to Specific Nodes
You can send data multiple times or to specific nodes using `ff_send_out`.

```py
from fastflow import FFAllToAll, EOS, ff_send_out

class source():
    def svc(self, *arg):
        # Send data items to sink1 and sink3
        ff_send_out(["source-to-sink2"], 1)
        ff_send_out(["source-to-sink3"], 2)
        # Send multiple data items to any node
        ff_send_out(["source-to-any"])
        ff_send_out(["source-to-any"])
        return EOS

class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(f"sink{self.id}")
        print(lis)

# All-to-All setup
a2a = FFAllToAll()
first_stage_size = 4
second_stage_size = 3

# Create and add first stage (sources)
first_lis = [source() for i in range(first_stage_size)]
a2a.add_firstset(first_lis)

# Create and add second stage (sinks)
second_lis = [sink(i+1) for i in range(second_stage_size)]
a2a.add_secondset(second_lis)

# Run All-to-All
a2a.run_and_wait_end()
```

### Composing Building Blocks

#### All-to-All Inside Pipeline

This example demonstrates combining All-to-All inside a Pipeline. The pipeline uses multiple stages where one stage is an All-to-All pattern.

```py
from fastflow import FFPipeline, FFAllToAll, EOS

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [f"data{self.counter}"]

class stage():
    def svc(self, lis: list):
        lis.append(f"stage")
        return lis

class sink():
    def svc(self, lis: list):
        lis.append("sink")
        print(lis)

# Inner All-to-All setup
a2a = FFAllToAll()
first_lis = [stage() for _ in range(3)]
second_lis = [stage() for _ in range(2)]
a2a.add_firstset(first_lis)
a2a.add_secondset(second_lis)

# Pipeline setup
pipeline = FFPipeline()
pipeline.add_stage(source())
pipeline.add_stage(a2a)  # All-to-All as a stage in the pipeline
pipeline.add_stage(sink())

# Run pipeline
pipeline.run_and_wait_end()
```

#### Pipeline Inside All-to-All
This example shows Pipeline inside an All-to-All stage, combining the two patterns.

```py
from fastflow import FFAllToAll, FFPipeline, EOS

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [f"data{self.counter}"]

class stage():
    def svc(self, lis: list):
        lis.append(f"stage")
        return lis

class sink():
    def svc(self, lis: list):
        lis.append(f"sink")
        print(lis)

# build a Pipeline to be used inside the second stage of All-to-All
def build_pipeline():
    pipeline = FFPipeline()
    pipeline.add_stage(stage())
    pipeline.add_stage(stage())
    pipeline.add_stage(sink())
    return pipeline

# All-to-All setup
a2a = FFAllToAll()
first_lis = [source() for _ in range(3)]  # 3 first stages
second_lis = [build_pipeline() for _ in range(2)]  # 2 Pipelines in the second stage
a2a.add_firstset(first_lis)
a2a.add_secondset(second_lis)

# Run All-to-All with embedded pipelines
a2a.run_and_wait_end()
```

## Blocking mode
The `blocking_mode(boolean)` function allows you to control whether the nodes block and wait for input or continuously check for data. Setting `blocking_mode(True)` enables blocking mode, reducing resource usage, while `blocking_mode(False)` disables it, making the nodes more responsive but more resource-hungry. By default, building blocks have blocking mode disabled. All the building blocks support it, this is an example using a Farm:

```py
from fastflow import FFFarm, EOS

class source():
    def __init__(self):
        self.counter = 1

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [f"data{self.counter}"]

class worker():
    def svc(self, lis: list):
        lis.append(f"processed")
        return lis

class sink():
    def svc(self, lis: list):
        print(lis)

# Create a Farm with blocking mode enabled
farm = FFFarm()
farm.blocking_mode(True)  # Enable blocking mode

sourcenode = source()
sinknode = sink()
workers = [worker() for _ in range(3)]

farm.add_emitter(sourcenode)
farm.add_workers(workers)
farm.add_collector(sinknode)

farm.run_and_wait_end()
```

## Mapping and unmapping to cores

By default, each nodes is set to run to one and only one core, improving performances. The `no_mapping()` function disables FastFlow's feature of pinning nodes to specific cores, allowing more flexibility in resource allocation but potentially increasing overhead due to movement between cores. All the building blocks support it, this is an example using a Pipeline:

```py
from fastflow import FFPipeline, EOS

class source():
    def __init__(self):
        self.counter = 1

    def svc(self):
        if self.counter > 5:
            return EOS
        self.counter += 1
        return [f"data{self.counter}"]

class stage():
    def svc(self, lis: list):
        lis.append(f"processed")
        return lis

class sink():
    def svc(self, lis: list):
        lis.append("final")
        print(lis)

# Create a Pipeline with no mapping
pipeline = FFPipeline()
pipeline.no_mapping()  # Disable core pinning for nodes

pipeline.add_stage(source())
pipeline.add_stage(stage())
pipeline.add_stage(sink())

pipeline.run_and_wait_end()
```

## Contributing

Contributions are welcome! Please submit pull requests or open issues to help improve this project.
