# $Id: Makefile,v 1.10 2002/11/15 03:56:32 derekm Exp $

BUILD = ../../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =	SipTortureTests.cxx \
		test1.cxx \
		test2.cxx \
	        testSipStack1.cxx \
	        testSipMessage.cxx \
	        testParserCategories.cxx \
	        testDnsResolver.cxx \
	        testPreparse.cxx \
		testSipMessageMemory.cxx \
	        testTcpTransport.cxx \
		testTransactionFSM.cxx \
		lg.cxx

SRC = \
		Register.cxx \
		Transceiver.cxx \
		Registrar.cxx \
		InviteClient.cxx \
		InviteServer.cxx \
		Resolver.cxx	

#	        testNonInviteClientTx.cxx \
#		testNameAddrParamExclusions.cxx \

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


include $(BUILD)/Makefile.post
