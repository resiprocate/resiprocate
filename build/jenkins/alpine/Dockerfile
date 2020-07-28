FROM alpine:3.12

RUN apk add --no-cache autoconf-archive automake boost-dev g++ gcc gperf linux-headers libtool make openssl-dev pkgconfig && \
    adduser --system --uid 1000 jenkins

USER jenkins
