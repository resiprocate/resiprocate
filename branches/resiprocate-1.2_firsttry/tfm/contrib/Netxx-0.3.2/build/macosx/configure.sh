#!/bin/sh
TLS=""
IPV6=""

DARWIN_VERSION=`uname -v | awk '{print $4}' | sed 's/\..*//'`
if [ "x$DARWIN_VERSION" = "x5" ]; then
    IPV6="--disable-ipv6"
fi

if [ -r /usr/include/openssl/ssl.h ]; then
    TLS="--enable-tls --openssl-prefix=/usr"
elif [ -r /sw/include/openssl/ssl.h ]; then
    TLS="--enable-tls --openssl-prefix=/sw"
fi

cd ../..
perl configure.pl --disable-shared --disable-streambuf $TLS $IPV6 $*
