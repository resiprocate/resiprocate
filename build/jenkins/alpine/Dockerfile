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
                       python3-dev \
                       subversion \
                       telepathy-qt-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

RUN cd /root && \
    svn checkout https://svn.code.sf.net/p/cxx/code/tags/7.1.4 pycxx && \
    cd pycxx && \
    python3 setup.py install && \
    cd .. && \
    rm -fr pycxx

USER jenkins
