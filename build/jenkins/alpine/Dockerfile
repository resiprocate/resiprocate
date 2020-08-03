FROM alpine:3.12

RUN apk add --no-cache asio-dev \
                       autoconf-archive \
                       automake \
                       boost-dev \
                       c-ares-dev \
                       cppunit-dev \
                       db-dev \
                       g++ \
                       gcc \
                       gperf \
                       linux-headers \
                       libtool \
                       make \
                       mariadb-connector-c-dev \
                       net-snmp-dev \
                       openssl-dev \
                       pcre-dev \
                       pkgconfig \
                       popt-dev \
                       postgresql-dev \
                       telepathy-qt-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

USER jenkins
