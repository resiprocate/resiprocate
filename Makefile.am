

AUTOMAKE_OPTIONS = foreign dist-tarZ
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = rutil
SUBDIRS += resip
SUBDIRS += repro
if BUILD_TFM
SUBDIRS += tfm
endif

# The Makefiles for these don't separate SSL stuff yet...
if USE_SSL
SUBDIRS += reTurn reTurn/client
SUBDIRS += presSvr
if BUILD_P2P
SUBDIRS += p2p/s2c/s2c p2p
endif
endif

EXTRA_DIST = build-contrib.sh

EXTRA_DIST += reSIProcate_10_0.sln
EXTRA_DIST += reSIProcate_7_1.sln
EXTRA_DIST += reSIProcate_8_0.sln
EXTRA_DIST += reSIProcate_9_0.sln

EXTRA_DIST += resip.spec

EXTRA_DIST += RELEASE-PROCESS.txt

# reSIProcate includes a modified version of ares
# libc-ares can also be used (e.g. in Debian)
# it is included in contrib and NOT automatically
# build by autotools, if it is needed, it is necessary
# to go into contrib/ares and build it before building
# the main stack
EXTRA_DIST += contrib/ares

