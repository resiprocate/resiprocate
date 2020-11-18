#!/bin/bash

set -e

# This scripts configures to build in a Debian environment

if dpkg-query -s libradcli-dev >/dev/null 2>&1 ;
then
  RADIUS_LIB=--with-radcli
elif dpkg-query -s libfreeradius-client-dev >/dev/null 2>&1 ;
then
  RADIUS_LIB=--with-freeradius
else
  echo "Please install libradcli-dev or libfreeradius-client-dev"
  exit 1
fi

autoreconf --install

# Qt libraries are required for the Telepathy Connection Manager
QT_VERSION=qt4
QT_LIB_PREFIX="/usr/include/${QT_VERSION}"
QT_LIBS="-I${QT_LIB_PREFIX}"
for lib in QtCore QtNetwork QtDBus ;
do
  QT_LIBS="${QT_LIBS} -I${QT_LIB_PREFIX}/${lib}"
done

OPENSSL_11="${HOME}/ws/openssl/debian/openssl-1.1.0c"
# the location where OpenSSL 1.1 headers are placed by "make install"
OPENSSL_11_INC="${OPENSSL_11}/debian/libssl-dev/usr/include"
# the location where OpenSSL 1.1 libs are placed by "make install"
OPENSSL_11_LIB="${OPENSSL_11}/debian/libssl1.1/usr/lib/x86_64-linux-gnu"

CFLAGS='-g -O0 -fPIE -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security' \
CPPFLAGS="-D_FORTIFY_SOURCE=2 -I${OPENSSL_11_INC} -I/usr/include/telepathy-qt4 ${QT_LIBS} -I/usr/include/postgresql -I/usr/include/sipxtapi -I/usr/include/gloox -D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFS -DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=20 -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS" \
CXXFLAGS='-g -O0 -fPIE -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive' \
LDFLAGS='-fPIE -pie -Wl,-z,relro -Wl,-z,now -lcares -L${OPENSSL_11_LIB}' \
  ./configure --disable-maintainer-mode --disable-dependency-tracking --with-popt --enable-ipv6 --enable-dtls $RADIUS_LIB --with-ssl \
              --enable-assert-syslog \
              --with-c-ares \
              --with-mysql \
              --with-postgresql \
              --with-repro \
              --enable-repro-plugins \
              --with-python \
                DEPS_PYTHON_CFLAGS="`/usr/bin/python2.7-config --cflags`" \
                DEPS_PYTHON_LIBS="`/usr/bin/python2.7-config --ldflags`" \
                PYCXX_SRCDIR=/usr/share/python2.7/CXX/Python2 \
              --with-apps \
              --with-telepathy \
              --with-ichat-gw \
              --with-recon


