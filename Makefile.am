
EXTRA_DIST = testua_readme.txt

bin_PROGRAMS = testUA

testUA_SOURCES = testUA.cxx
testUA_SOURCES += reConServer.hxx
testUA_SOURCES += reConServerConfig.cxx
testUA_SOURCES += reConServerConfig.hxx
testUA_SOURCES += playback_prompt.h
testUA_SOURCES += record_prompt.h

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


