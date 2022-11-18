#!/bin/bash

# 
# - for consistency, it is recommended that this script is always
#   run on the same platform (at least where the minor release number
#   remains the same between two releases), otherwise there is a risk
#   that a different version of autotools may produce Makefiles
#   that vary from what has already been tested.
#
# - at the moment, the supported platform for official bootstraps
#
#                Debian 11.x (bullseye)
#
# - to avoid errors about the `zip' and `compress' commands, run
#   the following:
#
#                apt-get install zip ncompress
#
# - note that the configure command below attempts to enable every optional
#   component.  If this is not done, `make dist' will not distribute
#   those components
#

if ! git diff --quiet ;
then
  echo "Uncommitted changes detected, please resolve"
  exit 1
fi

if ! git diff --cached --quiet ;
then
  echo "Staged and uncommitted changes detected, please resolve"
  exit 1
fi

autoreconf --install && \
  ./configure --with-popt --enable-ipv6 --enable-dtls --with-radcli --with-ssl \
              --enable-assert-syslog \
              --enable-android \
              --with-c-ares \
              --with-fmt \
              --with-mysql \
              --with-postgresql \
              --with-repro \
              --with-return \
              --enable-dso-plugins \
              --with-python \
                DEPS_PYTHON_VERSION=`python3 -c "import sys; print('%d.%d' % (sys.version_info[0], sys.version_info[1]))"` \
                DEPS_PYTHON_CFLAGS="`/usr/bin/python3-config --cflags`" \
                DEPS_PYTHON_LIBS="`/usr/bin/python3-config --ldflags`" \
                PYCXX_SRCDIR=/usr/src/CXX/Python3 \
              --with-rend \
              --with-tfm \
              --with-apps \
              --with-telepathy \
              --with-ichat-gw \
              --with-recon \
              --with-sipxtapi \
              --with-soci-postgresql \
              --with-soci-mysql \
              --with-qpid-proton \
              --with-geoip \
              --with-sigcomp \
              --with-netsnmp \
              --with-gstreamer \
              --with-p2p && \
  make dist


