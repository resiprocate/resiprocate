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

source build/distcc-setup

# -Wweak-vtables : use with clang to find classes without a key function

# For using the rutil/dns/ares library,
# add this to CPPFLAGS, the includes must come before /usr/include:
# -I`pwd`/rutil/dns/ares -DHAVE_ARPA_NAMESER_H -DHAVE_ARPA_NAMESER_COMPAT_H
# and remove -lcares from LDFLAGS

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
    -DBUILD_QPID_PROTON=ON \
    -DRESIP_ASSERT_SYSLOG=ON \
    .

