# $Id: Makefile,v 1.47 2003/08/13 16:45:25 fluffy Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD
LDLIBS += -lncurses

#CXXFLAGS += -I/usr/local/ssl/include
#LDFLAGS  += -L/usr/local/ssl/lib 
#LDLIBS   += /usr/local/ssl/lib/libssl.a /usr/local/ssl/lib/libcrypto.a 

TESTPROGRAMS =	limpc.cxx \
		testParseBuffer.cxx \
	        testData.cxx \
	        testDataStream.cxx \
		testCountStream.cxx \
	        testDataPerformance.cxx \
	        testRandomHex.cxx \
	        testCoders.cxx \
		testEmptyHeader.cxx \
		testThreadIf.cxx \
		testLogger.cxx \
		testXMLCursor.cxx \
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
		testApplicationSip.cxx \
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
