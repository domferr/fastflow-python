from fastflow_module import FFAllToAll, FFPipeline, EOS

"""
    source1 - stage11 - stage21 _   _ sink1
                                 | |
    source2 - stage12 - stage22 _|_|_ sink2
                                 | |
    source3 - stage13 - stage23 _|_|_ sink3
                                 |
    source4 - stage14 - stage24 _|
"""

class source():
    def __init__(self, id):
        self.counter = 1
        self.id = id

    def svc(self, *arg):
        if self.counter > 5:
            return EOS
        self.counter += 1

        return list([self.id])

class stage():
    def __init__(self, id):
        self.id = id
    
    def svc(self, lis: list):
        lis.append(self.id)
        return lis
    
class sink():
    def __init__(self, id):
        self.id = id

    def svc(self, lis: list):
        lis.append(self.id)

if __name__ == "__main__":
    # initialize all-to-all
    a2a = FFAllToAll()

    # create the first set
    firstset = []
    for i in range(1, 5):
        # create a pipeline and add stages
        pipe = FFPipeline(use_subinterpreters)
        pipe.add_stage(source(f'{(i*10)+1}'))
        pipe.add_stage(stage(f'{(i*10)+2}'))
        pipe.add_stage(stage(f'{(i*10)+3}'))
        # add pipeline to first set
        firstset.append(pipe)

    # create the second set
    secondset = [sink(f'sink{i+1}') for i in range(4)]

    # add first and second sets to the all-to-all
    a2a.add_firstset(firstset)
    a2a.add_secondset(secondset)

    # run the all-to-all and wait for the end
    a2a.run_and_wait_end()

    # initialize the pipeline
    pipeline = FFPipeline()
    pipeline.add_stage(source("Source"))
    pipeline.add_stage(stage("Stage-1"))
    pipeline.add_stage(stage("Stage-2"))

    # run the pipeline and wait for the end
    pipeline.run_and_wait_end()