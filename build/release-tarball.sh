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
#                Debian 10.0 (buster)
#
# - to avoid errors about the `zip' and `compress' commands, run
#   the following:
#
#                apt-get install zip ncompress
#
# - note that the configure command explicitly enables every optional
#   component.  If this is not done, `make dist' will not distribute
#   those components
#

autoreconf --install && \
  ./configure --with-ssl \
              --with-c-ares \
              --with-mysql \
              --with-postgresql \
              --with-freeradius \
              --with-repro \
              --with-return \
              --enable-repro-plugins \
              --with-python \
                DEPS_PYTHON_CFLAGS="`/usr/bin/python3.7-config --cflags`" \
                DEPS_PYTHON_LIBS="`/usr/bin/python3.7-config --ldflags`" \
                PYCXX_SRCDIR=/usr/share/python3.7/CXX/Python3 \
              --with-tfm \
              --with-apps \
              --with-telepathy \
              --with-ichat-gw \
              --with-recon \
              --with-qpid-proton \
              --with-netsnmp \
              --with-p2p && \
  make dist


