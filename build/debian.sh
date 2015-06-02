#!/bin/bash

set -e

# This scripts configures to build in a Debian environment

autoreconf --install

CFLAGS='-g -O2 -fPIE -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security' \
CPPFLAGS='-D_FORTIFY_SOURCE=2 -I/usr/include/postgresql -I/usr/include/sipxtapi -D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFS -DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=20 -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS' \
CXXFLAGS='-g -O2 -fPIE -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fpermissive' \
LDFLAGS='-fPIE -pie -Wl,-z,relro -Wl,-z,now -lcares' \
  ./configure --disable-maintainer-mode --disable-dependency-tracking --with-popt --enable-ipv6 --enable-dtls --with-freeradius --with-ssl \
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
              --with-recon


