# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../../build
CONTRIB = ../../../contrib

include $(BUILD)/Makefile.pre

ifeq ($(USE_DTLS), yes)
PACKAGES += DTLS_SHIM 
TESTPROGRAMS += testshim.cxx
endif

PACKAGES += RESIP RUTIL ARES OPENSSL PTHREAD POPT

ifeq ($(USE_SSL), yes)
TESTPROGRAMS += makeSelfCert.cxx \
		testSecurity.cxx \
		testTls.cxx 
endif


TESTPROGRAMS += \
testEmptyHfv.cxx \
RFC4475TortureTests.cxx \
limpc.cxx \
test503Generator.cxx \
testAppTimer.cxx \
testApplicationSip.cxx \
testClient.cxx \
testConnectionBase.cxx \
testCorruption.cxx \
testDigestAuthentication.cxx \
testDtlsTransport.cxx \
testDns.cxx \
testEmbedded.cxx \
testEmptyHeader.cxx \
testExternalLogger.cxx \
testFlowId.cxx \
testIM.cxx \
testLockStep.cxx \
testMessageWaiting.cxx \
testMultipartMixedContents.cxx \
testMultipartRelated.cxx \
testParserCategories.cxx \
testPidf.cxx \
testPidf.cxx \
testPksc7.cxx \
testPlainContents.cxx \
testRSP-2.cxx \
testResponses.cxx \
testRlmi.cxx \
testSdp.cxx \
testSelect.cxx \
testSelectInterruptor.cxx \
testServer.cxx \
testSipFrag.cxx \
testSipMessage.cxx \
testSipMessageMemory.cxx \
testSipStack1.cxx \
testStack.cxx \
testTcp.cxx \
testTime.cxx \
testTimer.cxx \
testTransactionFSM.cxx \
testTuple.cxx \
testTypedef.cxx \
testUdp.cxx \
testUri.cxx \
testXMLCursor.cxx 

#testTransportSelector.cxx


SRC = 		TestSupport.cxx

dumpasn1: dumpasn1.c
	gcc -o dumpasn1 dumpasn1.c

include $(BUILD)/Makefile.post
