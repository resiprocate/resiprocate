FROM ubuntu:focal

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get -y update && \
    apt-get -y upgrade && \
    apt-get -y install autoconf-archive \
                       automake \
                       g++ \
                       gcc \
                       gperf \
                       libasio-dev \
                       libboost-all-dev \
                       libc-ares-dev \
                       libdb++-dev \
                       libmysqlclient-dev \
                       libpcre3-dev \
                       libpopt-dev \
                       libpq-dev \
                       libsnmp-dev \
                       libssl-dev \
                       libtool \
                       make \
                       pkg-config && \
    adduser --system --uid 1000 jenkins

ENV CPATH="/usr/include/postgresql:${CPATH}"

USER jenkins
