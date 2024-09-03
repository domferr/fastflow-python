.PHONY: clean config repara-copy titanic-copy build

clean:
	rm -rf build

repara-copy:
	rsync -av -e ssh --exclude 'fastflow' --exclude '.git' --exclude '.venv' --exclude '*.egg-info' ./ dferraro@repara.unipi.it:/home/dferraro/fastflow-python

titanic-copy:
	rsync -av -e ssh --exclude 'fastflow' --exclude '.git' --exclude '.venv' --exclude '*.egg-info' ./ dferraro@titanic.unipi.it:/home/dferraro/fastflow-python

build: clean
	pip install .

