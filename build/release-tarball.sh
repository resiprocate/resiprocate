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
#                Debian 6.0 (squeeze)
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
              --with-radius \
              --with-tfm \
              --with-apps \
              --with-recon \
              --with-p2p && \
  make dist


