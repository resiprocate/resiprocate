# $Id: Makefile,v 1.26 2002/12/15 22:26:53 fluffy Exp $

USE_SSL = 1

BUILD = ../../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =	\
		testIM.cxx \
		testUri.cxx \
		SipTortureTests.cxx \
		testTlsConnection.cxx \
		lg.cxx \
		test1.cxx \
		test2.cxx \
		testDigestAuthentication.cxx \
		testMessageWaiting.cxx \
		testPlainContents.cxx \
		testSdp.cxx \
		testSipFrag.cxx \
		testSipMessageMemory.cxx \
		testSpeed.cxx \
		testTimer.cxx \
		testTransactionFSM.cxx \
	        testParserCategories.cxx \
	        testPreparse.cxx \
	        testSipMessage.cxx \
	        testSipStack1.cxx \
	        testTcpTransport.cxx \
		testMultipartMixedContents.cxx \
#	        testDnsResolver.cxx \


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
