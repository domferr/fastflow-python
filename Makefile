clean:
	rm -rf build

config:
	pip install setuptools

repara-copy:
	rsync -av -e ssh --exclude 'fastflow' --exclude '.git' --exclude '.venv' ./ dferraro@repara.unipi.it:/home/dferraro/fastflow-python

titanic-copy:
	rsync -av -e ssh --exclude 'fastflow' --exclude '.git' --exclude '.venv' ./ dferraro@titanic.unipi.it:/home/dferraro/fastflow-python

build: clean
	python3.12 setup.py install

