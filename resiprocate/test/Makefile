# $Id: Makefile,v 1.51 2003/09/14 08:27:16 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL USE_ARES PTHREAD
LDLIBS += -lncurses

#CXXFLAGS += -I/usr/local/ssl/include
#LDFLAGS  += -L/usr/local/ssl/lib 
#LDLIBS   += /usr/local/ssl/lib/libssl.a /usr/local/ssl/lib/libcrypto.a 

TESTPROGRAMS =	limpc.cxx \
		testParseUtil.cxx \
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
		testDigestAuthentication.cxx \
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
		testLockStep.cxx \
		testMultipartMixedContents.cxx \
		testClient.cxx \
		testServer.cxx \

#	        testTcpTransport.cxx \
#		lg.cxx \
#		test1.cxx \
#		test2.cxx \

SRC = 		TestSupport.cxx

#		Register.cxx \
#		Transceiver.cxx \
#		Registrar.cxx \
#		InviteClient.cxx \
#		InviteServer.cxx \

#		Resolver.cxx	

#	        testNonInviteClientTx.cxx \
#		testNameAddrParamExclusions.cxx \


include $(BUILD)/Makefile.post
