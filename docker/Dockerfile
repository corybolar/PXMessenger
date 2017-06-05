FROM ubuntu:xenial

RUN apt-get update -q && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get update -q && apt-get install -y libqt5gui5 \
	qtbase5-dev \
	libqt5multimedia5 \
	qtmultimedia5-dev \
	libevent-2.0-5 \
	libevent-dev \
	g++-5 \
	make \
	git \
	qt5-default \
	g++

RUN git clone https://github.com/cbpeckles/pxmessenger

ENTRYPOINT /bin/bash
