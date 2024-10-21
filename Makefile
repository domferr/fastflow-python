.PHONY: clean config repara-copy titanic-copy build ci

# run source .venv/bin/activate before running make build
build: clean
	pip install .

ci: clean
	cibuildwheel --platform linux

clean:
	rm -rf build
	rm src/fastflow.egg-info/ -rf

# example to run all the tests
# for file in $(ls tests/python/*.py); do echo "$file" && python3.12 $file; done