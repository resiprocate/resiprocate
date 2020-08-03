FROM alpine:3.12

RUN apk add --no-cache abi-compliance-checker \
                       asio-dev \
                       autoconf-archive \
                       automake \
                       boost-dev \
                       c-ares-dev \
                       cmake \
                       cppunit-dev \
                       db-dev \
                       g++ \
                       gcc \
                       git \
                       gnutls-dev \
                       gperf \
                       linux-headers \
                       libtool \
                       make \
                       mariadb-connector-c-dev \
                       net-snmp-dev \
                       nettle-dev \
                       openssl-dev \
                       pcre-dev \
                       pkgconfig \
                       popt-dev \
                       postgresql-dev \
                       python3-dev \
                       subversion \
                       telepathy-qt-dev \
                       xerces-c-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

WORKDIR /root

RUN svn checkout https://svn.code.sf.net/p/cxx/code/tags/7.1.4 pycxx && \
    cd pycxx && \
    python3 setup.py install && \
    cd .. && \
    rm -fr pycxx

RUN git clone https://github.com/radcli/radcli.git && \
    cd radcli && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make check && \
    make install && \
    cd .. && \
    rm -fr radcli

RUN git clone https://github.com/SOCI/soci.git && \
    cd soci && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && \
    cmake --build . && \
    cmake --install . && \
    cd ../.. && \
    rm -fr soci

RUN git clone https://github.com/cisco/libsrtp.git && \
    cd libsrtp && \
    git checkout tags/v1.6.0 && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr libsrtp

#RUN apk add --no-cache util-linux-dev && \
#    git clone https://github.com/apache/qpid-proton.git && \
#    cd qpid-proton && \
#    mkdir build && \
#    cd build && \
#    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && \
#    cmake --build . && \
#    cmake --install . && \
#    cd ../.. && \
#    rm -fr qpid-proton

RUN svn checkout svn://svn.camaya.net/gloox/branches/1.0 gloox && \
    cd gloox && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr gloox

USER jenkins
WORKDIR /home/jenkins
