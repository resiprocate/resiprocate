

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
SUBDIRS += p2p/s2c/s2c p2p
endif

EXTRA_DIST = build-contrib.sh

EXTRA_DIST += reSIProcate_10_0.sln
EXTRA_DIST += reSIProcate_7_1.sln
EXTRA_DIST += reSIProcate_8_0.sln
EXTRA_DIST += reSIProcate_9_0.sln

EXTRA_DIST += resip.spec

EXTRA_DIST += RELEASE-PROCESS.txt

