# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../build
CONTRIB = ../../contrib

include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE ARES OPENSSL PTHREAD POPT

ifeq ($(USE_SSL), true)
TESTPROGRAMS += makeSelfCert.cxx \
		testTls.cxx 
endif

TESTPROGRAMS += testMD5Stream.cxx \
		testTuple.cxx \
		test503Generator.cxx \
		limpc.cxx \
		testConnectionBase.cxx \
		SipTortureTests.cxx \
		testRSP-2.cxx \
		testPidf.cxx \
		testPksc7.cxx \
		testCorruption.cxx \
		testMem.cxx \
		testRlmi.cxx \
		testMultipartRelated.cxx \
		testDns.cxx \
		testDnsUtil.cxx \
		testFifo.cxx \
		testFileSystem.cxx \
		testTcp.cxx \
		testUdp.cxx \
		testStack.cxx \
		testResponses.cxx \
		testApplicationSip.cxx \
		testClient.cxx \
		testCountStream.cxx \
		testDigestAuthentication.cxx \
		testEmbedded.cxx \
		testEmptyHeader.cxx \
		testFlowId.cxx \
		testIM.cxx \
		testSHA1Stream.cxx \
		testIntrusiveList.cxx \
		testLockStep.cxx \
		testLockStep.cxx \
		testLogger.cxx \
		testExternalLogger.cxx \
		testMessageWaiting.cxx \
		testMultipartMixedContents.cxx \
		testParseBuffer.cxx \
		testPidf.cxx \
		testPlainContents.cxx \
		testSdp.cxx \
		testSelect.cxx \
		testServer.cxx \
		testSipFrag.cxx \
		testSipMessageMemory.cxx \
		testThreadIf.cxx \
		testTime.cxx \
		testTimer.cxx \
		testAppTimer.cxx \
		testTransactionFSM.cxx \
		testTypedef.cxx \
		testUri.cxx \
		testXMLCursor.cxx \
	        testCoders.cxx \
	        testData.cxx \
	        testDataPerformance.cxx \
	        testDataStream.cxx \
	        testParserCategories.cxx \
	        testRandomHex.cxx \
	        testSipMessage.cxx \
	        testSipStack1.cxx \
	        test503Generator.cxx \
		testSelectInterruptor.cxx \
#		testTransportSelector.cxx


SRC = 		TestSupport.cxx

dumpasn1: dumpasn1.c
	gcc -o dumpasn1 dumpasn1.c

include $(BUILD)/Makefile.post
