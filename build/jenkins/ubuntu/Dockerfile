FROM ubuntu:focal

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get -y update && \
    apt-get -y upgrade && \
    apt-get -y install autoconf-archive automake g++ gcc gperf libboost-all-dev libpopt-dev libssl-dev libtool make pkg-config && \
    adduser --system --uid 1000 jenkins

USER jenkins
