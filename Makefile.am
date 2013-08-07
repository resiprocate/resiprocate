
EXTRA_DIST = reConServer_readme.txt
EXTRA_DIST += reConServer.config

bin_PROGRAMS = reConServer

dist_man_MANS = reConServer.8

reConServer_SOURCES = reConServer.cxx
reConServer_SOURCES += reConServer.hxx
reConServer_SOURCES += reConServerConfig.cxx
reConServer_SOURCES += reConServerConfig.hxx
reConServer_SOURCES += playback_prompt.h
reConServer_SOURCES += record_prompt.h

LDADD = -lrecon
LDADD += -lreflow
LDADD += -lsipXsdp
LDADD += -lsipXmediaProcessing
LDADD += -lsipXmedia
LDADD += -lsipXport
LDADD += -lsipXtack
LDADD += -ldum
LDADD += -lresip
LDADD += -lrutil
LDADD += -lssl -lcrypto -lpthread


