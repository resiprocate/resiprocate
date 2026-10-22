#!/bin/bash

# Build sipXtapi with its autotools build and install it under a prefix.
#
# Usage: build/build_sipxtapi.sh SOURCE_DIR PREFIX [DEFAULT_BRIDGE_MAX_IN_OUTPUTS]
#
# On Linux the top-level CMakeLists.txt runs this as the build step of the
# sipXtapi ExternalProject (unless USE_SYSTEM_SIPXTAPI is on), after
# ExternalProject has cloned sipXtapi at the commit pinned in
# build/sipxtapi-commit.txt into SOURCE_DIR.  It can also be run by hand on
# any sipXtapi checkout.
#
# Headers are installed under PREFIX/include/sipxtapi and libraries under
# PREFIX/lib, the layout the Debian libsipxtapi-dev package used.
#
# The libraries recon needs are built, in dependency order: sipXportLib,
# sipXsdpLib, sipXtackLib, sipXmediaLib and sipXmediaAdapterLib.  recon links
# no sipXtackLib code, but sipXmediaAdapterLib's public headers include its
# net/*.h headers.  sipXcallLib is not needed.
#
# DEFAULT_BRIDGE_MAX_IN_OUTPUTS (default 10, sipXtapi's own default) must match
# the value recon is compiled with; CMake passes its DEFAULT_BRIDGE_MAX_IN_OUTPUTS
# cache value.
#
# The compiler is taken from CC and CXX, as for any autotools build.  With
# newer compilers this needs sipXtapi commit 343c125 (Sept 2026) or later:
# before it, sipXtackLib fails with g++ 11 and later, and sipXmediaLib and
# sipXtackLib fail with clang (tested: g++ 12, clang 16).

set -e

if [ $# -lt 2 ]
then
  echo "Usage: $0 SOURCE_DIR PREFIX [DEFAULT_BRIDGE_MAX_IN_OUTPUTS]" >&2
  exit 1
fi

SRC_DIR="$(cd "$1" && pwd)"
PREFIX="$2"
BRIDGE_MAX_IN_OUTPUTS="${3:-10}"

echo "*** Building sipXtapi in ${SRC_DIR}, installing into ${PREFIX} ***"

CORES=$(getconf _NPROCESSORS_ONLN)

# The configure scripts autoreconf generates from sipXtapi's m4 are missing
# some autoconf shell helpers (as_fn_exit, as_cr_alnum, ...), which breaks
# option validation and error handling.  Upstream carries a patch for each
# autoconf generation; sipXtapi's own Linux CI applies it the same way.
AUTOCONF_VERSION=$(autoconf --version | head -n 1 | awk '{print $NF}')
if [ "$(printf '%s\n' 2.70 "${AUTOCONF_VERSION}" | sort -V | head -n 1)" = "2.70" ]
then
  CONFIGURE_PATCH="${SRC_DIR}/configure-2.70+.patch"
else
  CONFIGURE_PATCH="${SRC_DIR}/configure.patch"
fi

export CPPFLAGS="-DDEFAULT_BRIDGE_MAX_IN_OUTPUTS=${BRIDGE_MAX_IN_OUTPUTS}"
export CFLAGS="-O1 -g"
export CXXFLAGS="-std=c++17 -O1 -g"

COMMON_FLAGS="--prefix=${PREFIX} --includedir=${PREFIX}/include/sipxtapi --with-pic --disable-doxygen"

# build_lib JOBS DIR [CONFIGURE_ARGS...]
build_lib()
{
  local jobs="$1"
  local lib="$2"
  shift 2
  echo "*** sipXtapi: building ${lib} ***"
  (
    cd "${SRC_DIR}/${lib}"
    local rebuild=false
    if [ -f Makefile ]
    then
      rebuild=true
    fi
    # Always regenerate: automake's own rebuild rules would regenerate
    # configure without the patch above and then fail running it.
    autoreconf -fi
    patch configure < "${CONFIGURE_PATCH}"
    # COMMON_FLAGS is deliberately word split
    ./configure ${COMMON_FLAGS} "$@"
    # Rebuilding a tree built before (e.g. after DEFAULT_BRIDGE_MAX_IN_OUTPUTS
    # changed) must not reuse objects compiled with the old flags, since make
    # does not track flag changes.  Only clean then: on a fresh tree
    # sipXmediaLib's clean fails in contrib directories not yet configured.
    if [ "${rebuild}" = true ]
    then
      make clean
    fi
    make -j"${jobs}"
    if [ "${lib}" = sipXmediaLib ]
    then
      # make install does not descend into the contrib speex sipXmediaLib is
      # configured to use, and libtool relinks the speex codec plugin against
      # the installed libraries while installing it, so install the contrib
      # speex first.  Otherwise the relink finds no libspeex/libspeexdsp (or
      # quietly picks up the system's, if libspeex-dev happens to be present).
      make -C contrib/libspeex install
    fi
    make install
  )
}

build_lib "${CORES}" sipXportLib --enable-shared --enable-static --without-cppunit
build_lib "${CORES}" sipXsdpLib
build_lib "${CORES}" sipXtackLib --disable-sipviewer
# The bundled speex does not survive a parallel make, so sipXmediaLib, which
# builds its contrib codecs as part of itself, is built with one job.
build_lib 1 sipXmediaLib \
  --disable-stream-player \
  --disable-local-audio \
  --enable-contrib-speex \
  --enable-speex-dsp \
  --enable-codec-speex \
  --enable-codec-g726 \
  --enable-codec-g722 \
  --enable-codec-g7221 \
  --enable-codec-g729 \
  --enable-codec-ilbc \
  --enable-codec-gsm \
  --enable-codec-opus
build_lib "${CORES}" sipXmediaAdapterLib \
  --disable-stream-player \
  --enable-topology-graph

echo "*** sipXtapi installed into ${PREFIX} ***"
