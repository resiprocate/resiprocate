# $Id: Makefile,v 1.29 2002/12/24 00:14:43 jason Exp $

USE_SSL = 1

BUILD = ../../../build

include $(BUILD)/Makefile.pre
-include ../../Makefile.opt

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 ARES PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TESTPROGRAMS =	\
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
		testMultipartMixedContents.cxx \
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
