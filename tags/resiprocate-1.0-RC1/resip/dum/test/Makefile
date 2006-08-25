# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../../../build
include $(BUILD)/Makefile.pre

PACKAGES += DUM RESIP RUTIL OPENSSL ARES PTHREAD POPT

TARGET_LIBRARY = 

#testDumTimer.cxx
TESTPROGRAMS += basicRegister.cxx BasicCall.cxx testSMIMEMessage.cxx basicMessage.cxx testSMIMEInvite.cxx

TESTPROGRAMS += limpc.cxx treg.cxx
SRC = CommandLineParser.cxx UserAgent.cxx

SRC += RegEventClient.cxx

ifeq ($(USE_CURL),yes)
TESTPROGRAMS += testIdentity.cxx
PACKAGES += CURL
SRC += curlHttp/CurlHttpProvider.cxx
CODE_SUBDIRS += curlHttp
endif

include $(BUILD)/Makefile.post
