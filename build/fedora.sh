#!/bin/bash

set -e

# This script configures to build in a Fedora environment

# some things are disabled in the Fedora builds because the dependencies
# are not packaged or not tested yet

# RHEL / Rocky Linux / CentOS users need to enable EPEL:

# sudo dnf install epel-release

# The typical command to install dependencies looks like this:

# dnf install gcc-c++ libtool automake autoconf asio-devel boost-devel cajun-jsonapi-devel c-ares-devel cppunit-devel gperf libdb-cxx-devel libdb-devel openssl-devel mariadb-connector-c-devel pcre-devel popt-devel postgresql-devel python3-devel python3-pycxx-devel radcli-devel xerces-c-devel net-snmp-devel qpid-proton-cpp-devel

# older systems may need slightly different permutations of the package names, for example, db4-cxx-devel db4-devel

RADIUS_LIB=--with-radcli
#RADIUS_LIB=--with-freeradius

autoreconf --install

source build/distcc-setup

CFLAGS='-fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -Wall -Wno-deprecated' \
CPPFLAGS="-I/usr/include -I/usr/include/gloox -D__pingtel_on_posix__ -D_linux_ -D_FILE_OFFS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DRESIP_DIGEST_LOGGING -DRECON_SDP_ENCODING_NAMES_CASE_HACK -I/usr/include/soci -I/usr/include/mysql `net-snmp-config --base-cflags`" \
CXXFLAGS='-fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive -Wall -Wno-deprecated' \
LDFLAGS='-fPIC -pie -Wl,-z,relro -Wl,-z,now -lcares' \
  cmake ${DISTCC} \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="${CPPFLAGS} ${CXXFLAGS}" \
    -DCMAKE_C_FLAGS="${CPPFLAGS} ${CFLAGS}" \
    -DWITH_C_ARES=ON \
    -DWITH_SSL=ON \
    -DUSE_SIGCOMP=OFF \
    -DVERSIONED_SONAME=ON \
    -DENABLE_ANDROID=OFF \
    -DUSE_IPV6=ON \
    -DUSE_DTLS=ON \
    -DPEDANTIC_STACK=OFF \
    -DUSE_SOCI_POSTGRESQL=ON \
    -DUSE_SOCI_MYSQL=ON \
    -DUSE_POSTGRESQL=ON \
    -DUSE_MAXMIND_GEOIP=ON \
    -DRESIP_HAVE_RADCLI=ON \
    -DBUILD_REPRO=ON \
    -DBUILD_REPRO_DSO_PLUGINS=ON \
    -DBUILD_RETURN=ON \
    -DBUILD_REND=ON \
    -DBUILD_TFM=OFF \
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
    -DPYCXX_SRCDIR=/usr/src/CXX/Python3 \
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
#                PYCXX_SRCDIR=/usr/src/CXX/Python3 \
#              --with-apps \
#              --with-recon \
#              --with-kurento \
#              --with-soci-postgresql \
#              --with-soci-mysql \
#              --with-qpid-proton \
#              --with-geoip \
#              --with-netsnmp \
#              --with-gstreamer


