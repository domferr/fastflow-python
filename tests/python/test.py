from fastflow import FFFarm, EOS
import argparse
import concurrent.futures

def svc():
    pass
    #print("svc")

class node():    
    def svc(self, *args):
        #print("svc")
        return EOS

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-workers', type=int, help='Number of workers of the farm', required=True)
    parser.add_argument('-usepool', help='Use ProcessPoolExecutor', required=False, action='store_true', default=False)
    args = parser.parse_args()
    if args.usepool:
        with concurrent.futures.ProcessPoolExecutor(max_workers=args.workers) as exe:
            # issue tasks to the process pool
            result = [exe.submit(svc) for _ in range(args.workers)]
        concurrent.futures.wait(result)
    else:
        farm = FFFarm(False)
        farm.add_workers([node() for _ in range(args.workers)])
        farm.run_and_wait_end()