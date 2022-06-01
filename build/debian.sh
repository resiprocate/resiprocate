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

source build/distcc-setup

CFLAGS='-g -O0 -fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security' \
CPPFLAGS="-D_FORTIFY_SOURCE=2 -I/usr/include/postgresql -I/usr/include/sipxtapi -I/usr/include/gloox -D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFS -DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=20 -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DRESIP_DIGEST_LOGGING -DRECON_SDP_ENCODING_NAMES_CASE_HACK -I/usr/include/soci -I/usr/include/mysql `net-snmp-config --base-cflags`" \
CXXFLAGS='-g -O0 -fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive' \
LDFLAGS='-fPIC -pie -Wl,-z,relro -Wl,-z,now -lcares' \
  ./configure \
              ${DISTCC} \
              --with-popt \
              --enable-ipv6 \
              --enable-dtls \
              $RADIUS_LIB \
              --with-ssl \
              --enable-assert-syslog \
              --with-c-ares \
              --with-fmt \
              --with-mysql \
              --with-postgresql \
              --with-repro \
              --with-return \
              --enable-repro-plugins \
              --with-python \
                DEPS_PYTHON_VERSION=`python3 -c "import sys; print('%d.%d' % (sys.version_info[0], sys.version_info[1]))"` \
                DEPS_PYTHON_CFLAGS="`/usr/bin/python3-config --cflags`" \
                DEPS_PYTHON_LIBS="`/usr/bin/python3-config --ldflags`" \
                PYCXX_SRCDIR=/usr/src/CXX/Python3 \
              --with-apps \
              --with-ichat-gw \
              --with-recon \
              --with-sipxtapi \
              --with-kurento \
              --with-soci-postgresql \
              --with-soci-mysql \
              --with-qpid-proton \
              --with-geoip \
              --with-netsnmp \
              --with-gstreamer


