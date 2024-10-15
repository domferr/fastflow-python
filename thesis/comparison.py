import multiprocessing
from fastflow import FFPipeline, ff_send_out

# Stage 1: Generate numbers from 1 to 10
def number_generator(pipe_out):
    counter = 1
    while counter <= 10:
        pipe_out.send(counter)
        counter += 1
    pipe_out.close()

# Stage 2: Square each number
def square_node(pipe_in, pipe_out):
    while True:
        try:
            data = pipe_in.recv()
            pipe_out.send(data*data)
        except EOFError:
            pipe_out.close()
            break

# Stage 3: Square each number
def subtract_five_node(pipe_in, pipe_out):
    while True:
        try:
            data = pipe_in.recv()
            pipe_out.send(data-5)
        except EOFError:
            pipe_out.close()
            break

# Create the pipes
pipe1_out, pipe1_in = multiprocessing.Pipe(duplex=False)
pipe2_out, pipe2_in = multiprocessing.Pipe(duplex=False)

# Create the processes
processes = [
    multiprocessing.Process(target=number_generator, args=(pipe1_in)),
    multiprocessing.Process(target=square_node, args=(pipe1_out, pipe2_in)),
    multiprocessing.Process(target=subtract_five_node, args=(pipe2_out, None))
]

# Start all processes in the pipeline
for process in processes:
    process.start()

# Wait for all processes to finish
for process in processes:
    process.join()


class NumberGenerator():
    def svc(self):
        for i in range(1, 11):
            ff_send_out(i)
        return EOS

class SquareNode():
    def svc(self, number):
        return number * number

class SubtractFiveNode():
    def svc(self, number):
        return number - 5

# Build the pipeline and add the stages
pipe = FFPipeline()
pipe.add_stage(NumberGenerator())
pipe.add_stage(SquareNode())
pipe.add_stage(SubtractFiveNode())

# Run the pipeline
if pipe.run_and_wait_end() < 0:
    # handle error...