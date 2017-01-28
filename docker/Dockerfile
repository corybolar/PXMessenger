FROM ubuntu:xenial

RUN apt-get update -q
RUN apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get update -q
RUN apt-get install -y qt5-default qttools5-dev-tools
RUN apt-get install -y libqt5multimedia5 qtmultimedia5-dev
RUN apt-get install -y g++-5
RUN apt-get install -y libevent-dev
RUN apt-get install -y git
RUN apt-get install -y make

RUN git clone https://github.com/cbpeckles/pxmessenger
CMD cd ./pxmessenger && echo $PWD && qmake -qt=qt5 QMAKE_CXX=g++-5 QMAKE_LINK=g++-5 && make
