docker:
	docker build -t pythontest .
	docker run --rm -p 8888:8888 -it --name pythontest pythontest

copy:
	docker cp ./include pythontest:./
	docker cp bindings.cpp pythontest:./
	docker cp parallel.py pythontest:./
	docker cp ./obj.py pythontest:./
	docker cp ./test_ff_send_out.py pythontest:./

build:
	cmake -DCMAKE_BUILD_TYPE=Debug ../ && make -j 2 && cp fastflow.cpython-312-x86_64-linux-gnu.so ..
