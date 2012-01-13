

AUTOMAKE_OPTIONS = foreign dist-tarZ
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = contrib/ares rutil resip repro

# The Makefiles for these don't separate SSL stuff yet...
if USE_SSL
SUBDIRS += reTurn reTurn/client
SUBDIRS += presSvr
SUBDIRS += p2p/s2c/s2c p2p
endif


