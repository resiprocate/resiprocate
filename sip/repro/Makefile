# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD

TARGET_LIBRARY = librepro


TESTPROGRAMS = \
	repro.cxx \
	userAdmin.cxx 

CXXFLAGS += -I/sw/include
LDFLAGS  += -L/sw/lib 
LDLIBS   += -ldb-4.2


SRC =   \
	Proxy.cxx \
	UserDb.cxx \
	WebAdmin.cxx \
	RequestContext.cxx \
	ResponseContext.cxx \
        RequestProcessor.cxx \
        RequestProcessorChain.cxx



include $(BUILD)/Makefile.post
