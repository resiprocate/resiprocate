# $Id: Makefile,v 1.3 2004/04/11 00:59:48 davidb Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD
LDLIBS += -lncurses 

CXXFLAGS += -I/sw/include
LDFLAGS  += -L/sw/lib

# this is so that popt only works where there is popt
# To enable popt, add USE_POPT=true to sip/build/Makefile.conf
# This only applies to the non-autotools build system
ifdef USE_POPT
LDLIBS += -lpopt
TESTPROGRAMS +=
endif

TESTPROGRAMS += Dialog.cxx

SRC =   Dialog.cxx \
	DialogSet.cxx


include $(BUILD)/Makefile.post
