# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../../../build
include $(BUILD)/Makefile.pre

ifdef USE_POPT
LDLIBS += -lpopt
endif

PACKAGES += DUM RESIPROCATE OPENSSL ARES PTHREAD

TARGET_LIBRARY = 

TESTPROGRAMS = certServer.cxx
#basicRegister.cxx BasicCall.cxx #testDumTimer.cxx
SRC =   

include $(BUILD)/Makefile.post
