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
                       libcppunit-dev \
                       libdb++-dev \
                       libmysqlclient-dev \
                       libpcre3-dev \
                       libpopt-dev \
                       libpq-dev \
                       libradcli-dev \
                       libsipxtapi-dev \
                       libsnmp-dev \
                       libsoci-dev \
                       libssl-dev \
                       libtelepathy-qt5-dev \
                       libtool \
                       make \
                       pkg-config \
                       python3-cxx-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

ENV CPATH="/usr/include/postgresql:/usr/include/sipxtapi:${CPATH}"

USER jenkins
