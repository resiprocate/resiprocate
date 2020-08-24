FROM alpine:3.12

RUN apk add --no-cache abi-compliance-checker \
                       asio-dev \
                       autoconf-archive \
                       automake \
                       boost-dev \
                       c-ares-dev \
                       cmake \
                       coreutils \
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
                       net-tools \
                       nettle-dev \
                       openssl-dev \
                       opus-dev \
                       pcre-dev \
                       pkgconfig \
                       popt-dev \
                       postgresql-dev \
                       python3-dev \
                       qtchooser \
                       subversion \
                       tar \
                       telepathy-qt-dev \
                       util-linux-dev \
                       xerces-c-dev \
                       xxd && \
    adduser --system --uid 1000 jenkins

WORKDIR /root

RUN svn checkout https://svn.code.sf.net/p/cxx/code/tags/7.1.4 pycxx && \
    cd pycxx && \
    python3 setup.py install && \
    cd .. && \
    rm -fr pycxx

ADD https://github.com/radcli/radcli/archive/1.2.11.tar.gz /root/
RUN tar -xf 1.2.11.tar.gz && \
    rm 1.2.11.tar.gz && \
    cd radcli-1.2.11 && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make check && \
    make install && \
    cd .. && \
    rm -fr radcli-1.2.11

ADD https://github.com/SOCI/soci/archive/4.0.0.tar.gz /root/
COPY soci-code-fixes.patch /root/
RUN tar -xf 4.0.0.tar.gz && \
    rm 4.0.0.tar.gz && \
    cd soci-4.0.0 && \
    patch -p0 < ../soci-code-fixes.patch && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && \
    cmake --build . && \
    cmake --install . && \
    cd ../.. && \
    rm -fr soci-4.0.0 && \
    rm soci-code-fixes.patch

ADD https://github.com/cisco/libsrtp/archive/v1.6.0.tar.gz /root/
RUN tar -xf v1.6.0.tar.gz && \
    rm v1.6.0.tar.gz && \
    cd libsrtp-1.6.0 && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr libsrtp-1.6.0

RUN svn checkout svn://svn.camaya.net/gloox/branches/1.0 gloox && \
    cd gloox && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    cd .. && \
    rm -fr gloox

ADD https://github.com/sipXtapi/sipXtapi/archive/3.3.0_test18.tar.gz /root/
COPY sipXtapi-code-fixes.patch /root/
RUN tar -xf 3.3.0_test18.tar.gz && \
    rm 3.3.0_test18.tar.gz && \
    cd sipXtapi-3.3.0_test18 && \
    patch -p0 < ../sipXtapi-code-fixes.patch && \
    autoreconf --install && \
    ./configure --prefix=/usr --disable-codec-ilbc && \
    make CPPFLAGS='-DINCLUDE_SIPX_RESPARSE -DDEFAULT_CODECS_PATH=\"/usr/lib\"' && \
    make install && \
    cd .. && \
    rm -fr sipXtapi-3.3.0_test18 && \
    rm sipXtapi-code-fixes.patch

ADD https://github.com/apache/qpid-proton/archive/0.31.0.tar.gz /root/
COPY qpid-proton-code-fixes.patch /root/
RUN tar -xf 0.31.0.tar.gz && \
    rm 0.31.0.tar.gz && \
    cd qpid-proton-0.31.0 && \
    patch -p0 < ../qpid-proton-code-fixes.patch && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && \
    cmake --build . && \
    ctest . && \
    cmake --install . && \
    cd ../.. && \
    rm -fr qpid-proton-0.31.0 && \
    rm qpid-proton-code-fixes.patch

ADD http://ftp.debian.org/debian/pool/main/n/netxx/netxx_0.3.2.orig.tar.gz /root/
COPY netxx-code-fixes.patch /root/
RUN tar -xf netxx_0.3.2.orig.tar.gz && \
    rm netxx_0.3.2.orig.tar.gz && \
    cd Netxx-0.3.2 && \
    patch -p0 < ../netxx-code-fixes.patch && \
    ./configure.pl --prefix=/usr --disable-examples && \
    make && \
    make install && \
    cd .. && \
    rm -fr Netxx-0.3.2 && \
    rm netxx-code-fixes.patch

USER jenkins
WORKDIR /home/jenkins
