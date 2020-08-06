FROM ubuntu:focal

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get -y update && \
    apt-get -y upgrade && \
    apt-get -y install autoconf-archive \
                       automake \
                       g++ \
                       gcc \
                       git \
                       gperf \
                       libasio-dev \
                       libboost-all-dev \
                       libc-ares-dev \
                       libcppunit-dev \
                       libdb++-dev \
                       libgloox-dev \
                       libmysqlclient-dev \
                       libpcre3-dev \
                       libpopt-dev \
                       libpq-dev \
                       libqpid-proton-cpp12-dev \
                       libradcli-dev \
                       libsipxtapi-dev \
                       libsnmp-dev \
                       libsoci-dev \
                       libssl-dev \
                       libtelepathy-qt5-dev \
                       libtool \
                       libxerces-c-dev \
                       make \
                       pkg-config \
                       python3-cxx-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

WORKDIR /root

RUN git clone https://github.com/cisco/libsrtp.git && \
    cd libsrtp && \
    git checkout tags/v1.6.0 && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr libsrtp

ENV CPATH="/usr/include/postgresql:${CPATH}"

USER jenkins
WORKDIR /home/jenkins
