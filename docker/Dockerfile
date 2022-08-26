FROM docker.io/library/ubuntu:focal

RUN apt update && \
	apt install -y software-properties-common && apt update

RUN apt install -y net-tools vim git && \
	apt install -y build-essential zlib1g-dev cmake

RUN apt-get install -y libboost-all-dev
RUN apt install -y libtbb-dev
RUN apt install -y libgoogle-glog-dev libgflags-dev
RUN apt install -y pkg-config gdb

# prometheus-cpp
RUN git clone https://github.com/jupp0r/prometheus-cpp.git && \ 
	pushd prometheus-cpp && \
	git submodule init && \
	git submodule update && \
	pushd _build && \
	cd _build && cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF && \
	(cmake --build . --parallel 4) && \
	(ctest -V) && \
	(cmake --install .) && \
	popd && popd && rm -rf prometheus-cpp



#RUN	add-apt-repository -y ppa:ubuntu-toolchain-r/test &&\
#		apt install -y gcc-11 g++-11

#USER builder
