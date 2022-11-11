#!/bin/bash

set -e

# This scripts configures to build in a Debian environment
#
# To build with clang instead of gcc, do something like this:
#
#   CC=clang CXX=clang++ build/debian.sh
#

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

# -Wweak-vtables : use with clang to find classes without a key function

CFLAGS='-fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -Wall -Wno-deprecated' \
CXXFLAGS='-fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive -Wall -Wno-deprecated' \
CPPFLAGS="-D_FORTIFY_SOURCE=2 -I/usr/include/postgresql -I/usr/include/sipxtapi -I/usr/include/gloox -D__pingtel_on_posix__ -D_linux_ -D_FILE_OFFS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DRESIP_DIGEST_LOGGING -DRECON_SDP_ENCODING_NAMES_CASE_HACK -I/usr/include/soci -I/usr/include/mysql `net-snmp-config --base-cflags`" \
LDFLAGS='-fPIC -pie -Wl,-z,relro -Wl,-z,now -lcares' \
  cmake ${DISTCC} \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="${CPPFLAGS} ${CXXFLAGS}" \
    -DCMAKE_C_FLAGS="${CPPFLAGS} ${CFLAGS}" \
    -DWITH_C_ARES=ON \
    -DWITH_SSL=ON \
    -DUSE_POPT=ON \
    -DUSE_SIGCOMP=ON \
    -DUSE_FMT=OFF \
    -DVERSIONED_SONAME=ON \
    -DENABLE_ANDROID=OFF \
    -DUSE_IPV6=ON \
    -DUSE_DTLS=ON \
    -DPEDANTIC_STACK=OFF \
    -DUSE_MYSQL=ON \
    -DUSE_SOCI_POSTGRESQL=ON \
    -DUSE_SOCI_MYSQL=ON \
    -DUSE_POSTGRESQL=ON \
    -DUSE_MAXMIND_GEOIP=ON \
    -DRESIP_HAVE_RADCLI=ON \
    -DUSE_NETSNMP=ON \
    -DBUILD_REPRO=ON \
    -DBUILD_REPRO_DSO_PLUGINS=ON \
    -DBUILD_RETURN=ON \
    -DBUILD_REND=ON \
    -DBUILD_TFM=ON \
    -DBUILD_ICHAT_GW=OFF \
    -DBUILD_TELEPATHY_CM=OFF \
    -DBUILD_RECON=ON \
    -DUSE_SRTP1=OFF \
    -DBUILD_RECONSERVER=ON \
    -DUSE_SIPXTAPI=ON \
    -DUSE_KURENTO=ON \
    -DUSE_GSTREAMER=ON \
    -DUSE_LIBWEBRTC=OFF \
    -DRECON_LOCAL_HW_TESTS=OFF \
    -DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=20 \
    -DBUILD_P2P=OFF \
    -DBUILD_PYTHON=ON \
    -DPYCXX_SRCDIR=/usr/src/CXX \
    -DBUILD_QPID_PROTON=ON \
    -DRESIP_ASSERT_SYSLOG=ON \
    .
#  ./configure \
#              ${DISTCC} \
#              --with-popt \
#              --enable-ipv6 \
#              --enable-dtls \
#              $RADIUS_LIB \
#              --with-ssl \
#              --enable-assert-syslog \
#              --with-c-ares \
#              --with-fmt \
#              --with-mysql \
#              --with-postgresql \
#              --with-repro \
#              --with-return \
#              --enable-dso-plugins \
#              --with-python \
#                DEPS_PYTHON_VERSION=`python3 -c "import sys; print('%d.%d' % (sys.version_info[0], sys.version_info[1]))"` \
#                DEPS_PYTHON_CFLAGS="`/usr/bin/python3-config --cflags`" \
#                DEPS_PYTHON_LIBS="`/usr/bin/python3-config --ldflags`" \
#                PYCXX_SRCDIR=/usr/src/CXX \
#              --with-rend \
#              --with-tfm \
#              --with-apps \
#              --with-ichat-gw \
#              --with-recon \
#              --with-sipxtapi \
#              --with-kurento \
#              --with-soci-postgresql \
#              --with-soci-mysql \
#              --with-qpid-proton \
#              --with-geoip \
#              --with-sigcomp \
#              --with-netsnmp \
#              --with-gstreamer


