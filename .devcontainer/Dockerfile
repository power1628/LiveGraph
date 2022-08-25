FROM docker.io/library/ubuntu:focal

RUN apt update && \
	apt install -y software-properties-common && apt update

RUN apt install -y net-tools vim git && \
	apt install -y build-essential zlib1g-dev cmake

RUN apt-get install -y libboost-all-dev
RUN apt install -y libtbb-dev
RUN apt install -y libgoogle-glog-dev libgflags-dev
RUN apt install -y pkg-config gdb


#RUN	add-apt-repository -y ppa:ubuntu-toolchain-r/test &&\
#		apt install -y gcc-11 g++-11

#USER builder
