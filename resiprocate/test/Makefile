# $Id: Makefile,v 1.62 2003/10/17 03:52:32 wensong Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD
LDLIBS += -lncurses 

# this is so that popt only works where there is popt
# To enable popt, add USE_POPT=true to sip/build/Makefile.conf
# This only applies to the non-autotools build system
ifdef USE_POPT
LDLIBS += -lpopt
TESTPROGRAMS +=
endif

TESTPROGRAMS += testDns.cxx \
		testTcp.cxx \
		testUdp.cxx \
		testStack.cxx \
		testResponses.cxx \
		limpc.cxx \
		testApplicationSip.cxx \
		testClient.cxx \
		testCountStream.cxx \
		testDigestAuthentication.cxx \
		testEmbedded.cxx \
		testEmptyHeader.cxx \
		testIM.cxx \
		testIM.cxx \
		testIntrusiveList.cxx \
		testLockStep.cxx \
		testLockStep.cxx \
		testLogger.cxx \
		testMessageWaiting.cxx \
		testMultipartMixedContents.cxx \
		testParseBuffer.cxx \
		testParseUtil.cxx \
		testPlainContents.cxx \
		testSdp.cxx \
		testSelect.cxx \
		testServer.cxx \
		testSipFrag.cxx \
		testSipMessageMemory.cxx \
		testSpeed.cxx \
		testThreadIf.cxx \
		testTimer.cxx \
		testTransactionFSM.cxx \
		testTypedef.cxx \
		testUri.cxx \
		testXMLCursor.cxx \
	        testCoders.cxx \
	        testData.cxx \
	        testDataPerformance.cxx \
	        testDataStream.cxx \
	        testParserCategories.cxx \
	        testPreparse.cxx \
	        testRandomHex.cxx \
	        testSipMessage.cxx \
	        testSipStack1.cxx \

SRC = 		TestSupport.cxx


include $(BUILD)/Makefile.post
