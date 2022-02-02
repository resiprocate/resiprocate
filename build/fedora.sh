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

CFLAGS='-g -O2 -fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security' \
CPPFLAGS="-I/usr/include -I/usr/include/gloox -D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFS -DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=20 -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DRESIP_DIGEST_LOGGING -DRECON_SDP_ENCODING_NAMES_CASE_HACK -I/usr/include/soci -I/usr/include/mysql `net-snmp-config --base-cflags`" \
CXXFLAGS='-g -O2 -fPIC -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive' \
LDFLAGS='-fPIC -pie -Wl,-z,relro -Wl,-z,now -lcares' \
  ./configure --with-popt --enable-ipv6 --enable-dtls $RADIUS_LIB --with-ssl \
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
              --with-recon \
              --with-soci-postgresql \
              --with-soci-mysql \
              --with-qpid-proton \
              --with-geoip \
              --with-netsnmp \
              --with-gstreamer


