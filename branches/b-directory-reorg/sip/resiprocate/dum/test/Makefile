# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../../../build
include $(BUILD)/Makefile.pre

PACKAGES += DUM RESIPROCATE OPENSSL ARES PTHREAD POPT

TARGET_LIBRARY = 

#testDumTimer.cxx
TESTPROGRAMS += basicRegister.cxx BasicCall.cxx testSMIMEMessage.cxx basicMessage.cxx

TESTPROGRAMS += limpc.cxx
SRC = CommandLineParser.cxx UserAgent.cxx

ifeq ($(USE_CURL),true)
TESTPROGRAMS += testIdentity.cxx
PACKAGES += CURL
SRC += curlHttp/CurlHttpProvider.cxx
CODE_SUBDIRS += curlHttp
endif

include $(BUILD)/Makefile.post
