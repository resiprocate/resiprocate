# $Id: Makefile,v 1.20 2002/12/02 23:43:43 derekm Exp $

USE_SSL = 1

BUILD = ../../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =	\
		testSpeed.cxx \
		testPlainContents.cxx \
		testIM.cxx \
		testSipFrag.cxx \
		testMessageWaiting.cxx \
		testSdp.cxx \
		SipTortureTests.cxx \
		test1.cxx \
		test2.cxx \
	        testSipStack1.cxx \
	        testSipMessage.cxx \
	        testParserCategories.cxx \
	        testDnsResolver.cxx \
	        testPreparse.cxx \
		testSipMessageMemory.cxx \
		testTransactionFSM.cxx \
		lg.cxx \
#	        testTcpTransport.cxx \

SRC = \
		Register.cxx \
		Transceiver.cxx \
		Registrar.cxx \
		InviteClient.cxx \
		InviteServer.cxx \
		TestSupport.cxx \
		Resolver.cxx	

#	        testNonInviteClientTx.cxx \
#		testNameAddrParamExclusions.cxx \

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


include $(BUILD)/Makefile.post
