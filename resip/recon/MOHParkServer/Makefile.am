
# WARNING
#
# This is not a final version of Makefile.am
# 
# it has just been created to ensure the files are part of the
# distribution tarball so that they build on Windows
#

EXTRA_DIST = ca.pem
EXTRA_DIST += doc
EXTRA_DIST += *.vcproj
EXTRA_DIST += MOHParkServer.config
EXTRA_DIST += music.wav

LDADD = ../librecon.la
#LDADD += ../../........whatever

sbin_PROGRAMS = MOHParkServer

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

