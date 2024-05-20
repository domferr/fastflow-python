FROM ubuntu:23.10 

RUN apt-get update
RUN apt-get -y install python3.12 python3.12-venv python3.12-dev cmake build-essential cmake build-essential valgrind python3.12-dbg

#WORKDIR /usr/src/app

#COPY requirements.txt ./
#RUN pip install --no-cache-dir -r requirements.txt

COPY . .

RUN python3.12 -m venv .venv

RUN mkdir build
WORKDIR ./build

RUN cmake -DCMAKE_BUILD_TYPE=Debug ../
RUN make

RUN cp fastflow.cpython-312-x86_64-linux-gnu.so ../.venv/lib/python3.12/site-packages/

#CMD [ "python", "./your-daemon-or-script.py" ]
#CMD ["./py1"]

#source ../.venv/bin/activate