# $Id: Makefile,v 1.2 2002/11/07 02:49:16 derekm Exp $

BUILD = ../../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =  SipTortureTests.cxx test2.cxx testSipStack1.cxx testSipMessage.cxx testParserCategories.cxx testNonInviteClientTx.cxx testDnsResolver.cxx testPreparse.cxx testTcpTransport.cxx

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \

include $(BUILD)/Makefile.post
