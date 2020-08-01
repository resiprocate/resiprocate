FROM alpine:3.12

RUN apk add --no-cache autoconf-archive automake boost-dev c-ares-dev g++ gcc gperf linux-headers libtool make net-snmp-dev openssl-dev pkgconfig popt-dev && \
    adduser --system --uid 1000 jenkins

USER jenkins
