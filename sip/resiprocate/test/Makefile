# $Id: Makefile,v 1.30 2002/12/31 02:50:07 fluffy Exp $

USE_SSL = 1

BUILD = ../../../build

include $(BUILD)/Makefile.pre
-include ../../Makefile.opt

USE_REPO=false
PACKAGES += SIP2 SIPSTACK2 UTIL2 PTHREAD

ifeq ($(USE_ARES),1)
PACKAGES += ARES
endif

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

limpc: obj.debug.Linux.i686/limpc.o
	g++ -o limpc obj.debug.Linux.i686/limpc.o obj.debug.Linux.i686/Register.o obj.debug.Linux.i686/Transceiver.o obj.debug.Linux.i686/Registrar.o obj.debug.Linux.i686/InviteClient.o obj.debug.Linux.i686/InviteServer.o obj.debug.Linux.i686/TestSupport.o obj.debug.Linux.i686/Resolver.o -L../../../build/../sip2/sipstack/obj.debug.Linux.i686 -L../../../build/../sip2/util/obj.debug.Linux.i686 -lsip2 -lutil2 -lpthread -lssl -lcrypto -lncurses

include $(BUILD)/Makefile.post
