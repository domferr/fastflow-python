.PHONY: clean config repara-copy titanic-copy build ci

# run source .venv/bin/activate before running make build
build: clean
	pip install .

ci:
	cibuildwheel --platform linux

clean:
	rm -rf build