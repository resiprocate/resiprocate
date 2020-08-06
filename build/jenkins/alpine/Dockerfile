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

RUN svn checkout svn://svn.camaya.net/gloox/branches/1.0 gloox && \
    cd gloox && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr gloox

COPY sipXtapi-code-fixes.patch /root/
RUN apk add --no-cache coreutils net-tools opus-dev && \
    git clone https://github.com/sipXtapi/sipXtapi.git && \
    cd sipXtapi && \
    git checkout tags/3.3.0_test18 && \
    patch -p0 < ../sipXtapi-code-fixes.patch && \
    autoreconf --install && \
    ./configure --prefix=/usr --disable-codec-ilbc && \
    make CPPFLAGS='-DINCLUDE_SIPX_RESPARSE -DDEFAULT_CODECS_PATH=\"/usr/lib\"' && \
    make install && \
    cd .. && \
    rm -fr sipXtapi && \
    rm sipXtapi-code-fixes.patch

COPY qpid-proton-code-fixes.patch /root/
RUN apk add --no-cache util-linux-dev && \
    git clone https://github.com/apache/qpid-proton.git && \
    cd qpid-proton && \
    git checkout tags/0.31.0 && \
    patch -p0 < ../qpid-proton-code-fixes.patch && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && \
    cmake --build . && \
    ctest . && \
    cmake --install . && \
    cd ../.. && \
    rm -fr qpid-proton

USER jenkins
WORKDIR /home/jenkins
