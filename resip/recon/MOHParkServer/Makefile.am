
# WARNING
#
# This is not a final version of Makefile.am
# 
# it has just been created to ensure the files are part of the
# distribution tarball so that they build on Windows
#

EXTRA_DIST = ca.pem
EXTRA_DIST += doc
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += MOHParkServer.config
EXTRA_DIST += music.wav

LDADD = ../librecon.la
LDADD += ../../../reflow/libreflow.la
LDADD += ../../../reTurn/client/libreTurnClient.la
LDADD += ../../../media/libresipmedia.la
if USE_GSTREAMER
LDADD += $(GSTREAMERMM_1_0_LIBS) -lgstwebrtc-1.0
endif
LDADD += ../../../resip/dum/libdum.la
LDADD += ../../../resip/stack/libresip.la
LDADD += ../../../rutil/librutil.la
LDADD += -lsipXport
LDADD += $(LIBSSL_LIBADD) @LIBPTHREAD_LIBADD@

if USE_SIPXTAPI
sbin_PROGRAMS = MOHParkServer
endif

MOHParkServer_SOURCES = \
        AppSubsystem.cxx \
        ConfigParser.cxx \
        HttpBase.cxx \
        HttpConnection.cxx \
        MOHManager.cxx \
        MOHParkServer.cxx \
        ParkManager.cxx \
        ParkOrbit.cxx \
        Server.cxx \
        WebAdmin.cxx \
        WebAdminThread.cxx

noinst_HEADERS = \
        ActiveCallInfo.hxx \
        AppSubsystem.hxx \
        ConfigParser.hxx \
        HttpBase.hxx \
        HttpConnection.hxx \
        MOHManager.hxx \
        ParkManager.hxx \
        ParkOrbit.hxx \
        Server.hxx \
        WebAdmin.hxx \
        WebAdminThread.hxx


