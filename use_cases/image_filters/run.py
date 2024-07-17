from fastflow_module import FFFarm
from PIL import Image, ImageFilter
import argparse
import sys
import os
import shutil
import time
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import wait

class emitter():
    def __init__(self, images_path):
        self.images_path = images_path

    def svc(self, *args):
        if len(self.images_path) == 0:
            return None
        
        next = self.images_path.pop()
        return next

class worker():
    def __init__(self, id):
        self.id = id

    def svc(self, image_file):
        print(f"worker {self.id} -> {image_file}")

        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        image = Image.open(f'images/{image_file}')
        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"read took {res}ms")

        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        blurImage = image.filter(ImageFilter.GaussianBlur(20))
        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"blur took {res}ms")

        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        blurImage.save(f'images/blur/{self.id}_{image_file}')
        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"save took {res}ms")

        return image_file

class collector():
    def svc(self, *args):
        return args

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
    
    # add collector
    c = collector()
    if use_processes:
        farm.add_collector_process(c)
    else:
        farm.add_collector(c)

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
        def blur_filter(img):
            start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
            image = Image.open(f'images/{img}')
            end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

            res = (end - start) / 1_000_000
            print(f"read took {res}ms")

            start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
            blurImage = image.filter(ImageFilter.GaussianBlur(20))
            end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

            res = (end - start) / 1_000_000
            print(f"blur took {res}ms")

            start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
            blurImage.save(f'images/blur/proc{id}-{img}')
            end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

            res = (end - start) / 1_000_000
            print(f"save took {res}ms")
        
        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        with ProcessPoolExecutor(max_workers=args.workers) as exe:
            # issue tasks to the process pool
            futures = [exe.submit(blur_filter, img) for img in images_path ]
            wait(futures)

        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"python multiprocessing = {res}ms")