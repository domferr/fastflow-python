FROM ubuntu:23.10 

RUN apt-get update
RUN apt-get -y install python3.12 python3.12-venv python3.12-dev cmake build-essential cmake build-essential valgrind python3.12-dbg

WORKDIR app

COPY . .

RUN python3.12 -m venv .venv

#RUN source .venv/bin/activate && pip install setuptools
#RUN source .venv/bin/activate && python3 setup.py install
