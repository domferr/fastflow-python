from fastflow_module import FFFarm
from PIL import Image, ImageFilter
import argparse
import sys
import os
import shutil
import time
import multiprocessing

class emitter():
    def __init__(self, images_path):
        self.images_path = images_path

    def svc_init(self):
        print(f'[emitter] svc_init was called')
        return 0

    def svc(self, *args):
        print('[emitter] svc')
        if len(self.images_path) == 0:
            return None
        
        next = self.images_path.pop()
        return next

    def svc_end(self):
        print(f'[emitter] svc_end was called')

class worker():
    def __init__(self, id):
        self.id = id

    def svc_init(self):
        print(f'[{self.id} | worker] svc_init was called')
        return 0

    def svc(self, image_file):
        print(f'[{self.id} | worker] svc {image_file}')
        
        image = Image.open(f'images/{image_file}')
        blurImage = image.filter(ImageFilter.GaussianBlur(20))
        blurImage.save(f'images/blur/{self.id}_{image_file}')

        return args

    def svc_end(self):
        print(f'[{self.id} | worker] svc_end was called')

class collector():
    def svc_init(self):
        print(f'[collector] svc_init was called')
        return 0
    
    def svc(self, *args):
        print(f'[collector] svc')

        return args

    def svc_end(self):
        print(f'[collector] svc_end was called')

def build_farm(images_path, nworkers, use_processes = True, use_subinterpreters = False):
    farm = FFFarm(use_subinterpreters)

    # emitter
    em = emitter(images_path)
    if use_processes:
        farm.add_emitter_process(em)
    else:
        farm.add_emitter(em)

    # build workers list
    w_lis = []
    for i in range(nworkers):
        w = worker(f"{i+1}")
        w_lis.append(w)
    
    # add workers
    if use_processes:
        farm.add_workers_process(w_lis)
    else:
        farm.add_workers(w_lis)

    # collector
    coll = collector()
    if use_processes:
        farm.add_collector_process(coll)
    else:
        farm.add_collector(coll)

    return farm

def run_farm(images_path, nworkers, use_processes = False, use_subinterpreters = False):
    print(f"run farm of {nworkers} workers to apply image filter to {len(images_path)} images", file=sys.stderr)
    farm = build_farm(images_path, nworkers, use_processes, use_subinterpreters)
    farm.run_and_wait_end()
    return farm.ffTime()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('-images', type=int, help='Number of images to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=False, default=1)
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    group.add_argument('-seq', action='store_true', help='Run tasks sequentially')
    args = parser.parse_args()

    images_path = ["wallpaper.png" for _ in range(args.images)]

    if os.path.exists("images/blur"):
        shutil.rmtree("images/blur")
    os.makedirs("images/blur")

    if args.proc:
        res = run_farm(images_path, args.workers, use_processes = True, use_subinterpreters = False)
        print(f"done in {res}ms")

        print(f"processes = {res}ms, workers = {args.workers}")
    elif args.sub:
        res = run_farm(images_path, args.workers, use_processes = False, use_subinterpreters = True)
        print(f"done in {res}ms")

        print(f"subinterpreters = {res}ms, workers = {args.workers}")
    elif args.seq:
        print(f"Apply sequentially the image filter to {len(images_path)} images")
        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        for img in images_path:
            image = Image.open(f'images/{img}')
            blurImage = image.filter(ImageFilter.GaussianBlur(20))
            blurImage.save(f'images/blur/{img}')
        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"sequential = {res}ms")
    else:
        def process_fun(images, id):
            for img in images:
                image = Image.open(f'images/{img}')
                blurImage = image.filter(ImageFilter.GaussianBlur(20))
                blurImage.save(f'images/blur/proc{id}-{img}')
        
        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        # create a list for each process to put into it
        # the images the process will to work on
        proc_images = [[] for _ in range(args.workers)]
        # round-robin
        i = 0
        for img in images_path:
            proc_images[i].append(img)
            i = (i + 1) % args.workers

        # spawn processes
        processes = []
        for i in range(args.workers):
            proc = multiprocessing.Process(target=process_fun, args=(proc_images[i],i))
            processes.append(proc)
            proc.start()

        # join processes
        for proc in processes:
            proc.join()

        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"python multiprocessing = {res}ms")