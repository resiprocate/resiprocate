# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../build
CONTRIB = ../../../contrib

include $(BUILD)/Makefile.pre

PACKAGES += RUTIL ARES OPENSSL PTHREAD POPT

ifeq ($(USE_SSL), true)
TESTPROGRAMS += makeSelfCert.cxx \
		testTls.cxx 
endif

TESTPROGRAMS += \
	testCoders.cxx \
	testCountStream.cxx \
	testData.cxx \
	testDataPerformance.cxx \
	testDataStream.cxx \
	testDnsUtil.cxx \
	testFifo.cxx \
	testFileSystem.cxx \
	testInserter.cxx \
	testIntrusiveList.cxx \
	testLogger.cxx \
	testMD5Stream.cxx \
	testMem.cxx \
	testParseBuffer.cxx \
	testRandomHex.cxx \
	testSHA1Stream.cxx \
	testThreadIf.cxx \

#SRC = 	TestSupport.cxx

include $(BUILD)/Makefile.post
