from fastflow import FFFarm
from PIL import Image, ImageFilter
import argparse
import sys
import os
import shutil
import time
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import wait

def blur_filter(img):
    image = Image.open(f'images/{img}')
    blurImage = image.filter(ImageFilter.GaussianBlur(20))
    blurImage.save(f'images/blur/{img}')
    """start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
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
    blurImage.save(f'images/blur/{img}')
    end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

    res = (end - start) / 1_000_000
    print(f"save took {res}ms")"""

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
        blur_filter(image_file)
        return image_file

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

    return farm

def run_farm(images_path, nworkers, use_processes = False, use_subinterpreters = False, blocking_mode = None, no_mapping = False):
    print(f"run farm of {nworkers} workers to apply image filter to {len(images_path)} images", file=sys.stderr)
    farm = build_farm(images_path, nworkers, use_processes, use_subinterpreters)
    if blocking_mode is not None:
        farm.blocking_mode(blocking_mode)
    if no_mapping:
        farm.no_mapping()
    farm.run_and_wait_end()
    return farm.ffTime()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some tasks.')
    parser.add_argument('-images', type=int, help='Number of images to process', required=True)
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=False, default=1)
    parser.add_argument('-blocking-mode', help='The blocking mode of the farm', required=False, action='store_true', default=None)
    parser.add_argument('-no-mapping', help="Disable fastflow's mapping", required=False, action='store_true')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-proc', action='store_true', help='Use multiprocessing to process tasks')
    group.add_argument('-sub', action='store_true', help='Use subinterpreters to process tasks')
    group.add_argument('-seq', action='store_true', help='Run tasks sequentially')
    args = parser.parse_args()

    images_path = [f"wallpaper_{i}.png" for i in range(args.images)]
    for img in images_path:
        shutil.copy("images/wallpaper.png", f"images/{img}")

    if os.path.exists("images/blur"):
        shutil.rmtree("images/blur")
    os.makedirs("images/blur")

    if args.proc:
        res = run_farm(images_path, args.workers, use_processes = True, use_subinterpreters = False, blocking_mode = args.blocking_mode, no_mapping = args.no_mapping)
        print(f"done in {res}ms")

        print(f"processes = {res}ms, workers = {args.workers}")
    elif args.sub:
        res = run_farm(images_path, args.workers, use_processes = False, use_subinterpreters = True, blocking_mode = args.blocking_mode, no_mapping = args.no_mapping)
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
        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        with ProcessPoolExecutor(max_workers=args.workers) as exe:
            # issue tasks to the process pool
            futures = [exe.submit(blur_filter, img) for img in images_path ]
            wait(futures)

        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)

        res = (end - start) / 1_000_000
        print(f"python multiprocessing = {res}ms")