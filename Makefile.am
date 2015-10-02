

AUTOMAKE_OPTIONS = foreign dist-zip
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = rutil
SUBDIRS += resip
if BUILD_REPRO
SUBDIRS += repro
endif
if BUILD_TFM
SUBDIRS += tfm
endif

if BUILD_APPS
SUBDIRS += apps
endif

# The Makefiles for these don't separate SSL stuff yet...
if USE_SSL
SUBDIRS += reTurn
if BUILD_RECON
SUBDIRS += reflow
SUBDIRS += resip/recon
endif
if BUILD_P2P
SUBDIRS += p2p/s2c/s2c p2p
endif
endif

#EXTRA_DIST = build-contrib.sh

EXTRA_DIST = *.sln

EXTRA_DIST += resiprocate.spec

EXTRA_DIST += RELEASE-PROCESS.txt
EXTRA_DIST += build/configure-android.sh

