# $Id: Makefile,v 1.7 2002/11/13 17:07:39 jason Exp $

BUILD = ../../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =	SipTortureTests.cxx \
		test2.cxx \
	        testSipStack1.cxx \
	        testSipMessage.cxx \
	        testParserCategories.cxx \
	        testDnsResolver.cxx \
	        testPreparse.cxx \
		testSipMessageMemory.cxx \
	        testTcpTransport.cxx \
		testTransactionFSM.cxx 

#	        testNonInviteClientTx.cxx \
#		testNameAddrParamExclusions.cxx \

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \

include $(BUILD)/Makefile.post
