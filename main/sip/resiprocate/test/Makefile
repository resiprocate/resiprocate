# $Id: Makefile,v 1.35 2003/01/10 02:51:17 derekm Exp $

USE_SSL = 1

BUILD = ../../../build

include $(BUILD)/Makefile.pre
-include ../../Makefile.opt

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_ARES),1)
PACKAGES += ARES
endif

LDLIBS += -lncurses

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
CFLAGS += -DUSE_SSL
CXXFLAGS += -DUSE_SSL
endif

TESTPROGRAMS =	\
		testEmbedded.cxx \
		testIM.cxx \
		testUri.cxx \
		SipTortureTests.cxx \
		lg.cxx \
		test1.cxx \
		test2.cxx \
		testDigestAuthentication.cxx \
		testDnsResolver.cxx \
		testIM.cxx \
		testLockStep.cxx \
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
		testLockStep.cxx \
		testMultipartMixedContents.cxx \
		testClient.cxx \
		testServer.cxx \
#		testTlsConnection.cxx \


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


include $(BUILD)/Makefile.post
