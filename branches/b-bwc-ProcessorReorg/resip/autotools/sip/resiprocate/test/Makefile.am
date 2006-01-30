CXX = ${DISTCC} @CXX@
#AM_LDFLAGS = -lpopt
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

EXTRA_DIST = 	test_7_0.vcproj \
		test_7_1.vcproj \
		test_8_0.vcproj 

noinst_LIBRARIES = libtestsupport.a
libtestsupport_a_SOURCES = \
	InviteClient.cxx \
	InviteServer.cxx \
        makeSelfCert.cxx \
	Register.cxx \
	Registrar.cxx \
	TestSupport.cxx \
	test503Generator.cxx \
	testResponses.cxx \
	testRSP-2.cxx \
	testUdp.cxx

noinst_HEADERS = \
	digcalc.hxx \
	md5.hxx \
	Resolver.hxx \
	InviteClient.hxx \
	Register.hxx \
	testIM.hxx \
	InviteServer.hxx \
	Registrar.hxx \
	TestSupport.hxx \
	Transceiver.hxx \
	tassert.h

# The following files (.cxx) are not included as SOURCES
# anywhere in this makefile:
# 
# makeSelfCert (I know this is wrong)
#
# lg md5 test1 test2 testAor testBlast testConnectionBase
# testConvertRaw testCorruption testDigestAuthentication2
# testExternalBodyContents testForce testGperfHash
# testHack testHash testHashCasen testHeaderFieldValueList
# testId testIdentity testInserter testMacSecurity
# testMsgHeaderDiagram testNameAddrParamExclusions
# testNonInviteClientTx testNonInviteServerTx
# testParseNameAddr testParseUtil testSipStackInvite
# testSource testTls testTlsConnection testTransportSelector
# testTypes Transceiver

LDADD = libtestsupport.a ../libresiprocate.la
check_PROGRAMS = \
	SipTortureTests \
	testApplicationSip \
	testAppTimer \
	testClient \
	testCoders \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDigestAuthentication \
	testDns \
	testDnsUtil \
	testEmbedded \
	testEmptyHeader \
	testExternalLogger \
	testFifo \
	testFileSystem \
        testFlowId \
	testIM \
	testIntrusiveList \
        testLockStep \
	testLogger \
        testMD5Stream \
	testMem \
	testMessageWaiting \
	testMultipartMixedContents \
	testMultipartRelated \
	testParseBuffer \
	testParserCategories \
	testPidf \
	testPksc7 \
	testPlainContents \
	testRandomHex \
	testRlmi \
	testSHA1Stream \
	testSdp \
	testSelect \
	testSelectInterruptor \
	testServer \
	testSipFrag \
	testSipMessage \
	testSipMessageMemory \
	testSipStack1 \
	testStack \
	testTcp \
	testThreadIf \
	testTime \
	testTimer \
	testTransactionFSM \
	testTuple \
	testTypedef \
	testUri \
	testXMLCursor 

bin_PROGRAMS = limpc testTcp testDataPerformance

TESTS = $(check_PROGRAMS)
SipTortureTests_SOURCES = SipTortureTests.cxx
limpc_SOURCES = limpc.cxx
testApplicationSip_SOURCES = testApplicationSip.cxx
testAppTimer_SOURCES = testAppTimer.cxx
testClient_SOURCES = testClient.cxx
testCoders_SOURCES = testCoders.cxx
testCountStream_SOURCES = testCountStream.cxx
testData_SOURCES = testData.cxx
testDataPerformance_SOURCES = testDataPerformance.cxx
testDataStream_SOURCES = testDataStream.cxx
testDigestAuthentication_SOURCES = testDigestAuthentication.cxx
testDns_SOURCES = testDns.cxx
testDnsUtil_SOURCES = testDnsUtil.cxx
testEmbedded_SOURCES = testEmbedded.cxx
testEmptyHeader_SOURCES = testEmptyHeader.cxx
testExternalLogger_SOURCES = testExternalLogger.cxx
testFifo_SOURCES = 	testFifo.cxx
testFileSystem_SOURCES = testFileSystem.cxx
testFlowId_SOURCES = testFlowId.cxx
testIM_SOURCES = testIM.cxx
testIntrusiveList_SOURCES = testIntrusiveList.cxx
testLockStep_SOURCES = testLockStep.cxx
testLogger_SOURCES = testLogger.cxx
testMD5Stream_SOURCES = testMD5Stream.cxx
testMem_SOURCES = testMem.cxx
testMessageWaiting_SOURCES = testMessageWaiting.cxx
testMultipartMixedContents_SOURCES = testMultipartMixedContents.cxx
testMultipartRelated_SOURCES = testMultipartRelated.cxx
testParseBuffer_SOURCES = testParseBuffer.cxx
testParserCategories_SOURCES = testParserCategories.cxx
testPidf_SOURCES = testPidf.cxx
testPksc7_SOURCES = testPksc7.cxx
testPlainContents_SOURCES = testPlainContents.cxx
testRandomHex_SOURCES = testRandomHex.cxx
testRlmi_SOURCES = testRlmi.cxx
testSHA1Stream_SOURCES = testSHA1Stream.cxx
testSdp_SOURCES = testSdp.cxx
testSelect_SOURCES = testSelect.cxx
testSelectInterruptor_SOURCES = testSelectInterruptor.cxx
testServer_SOURCES = testServer.cxx
testSipFrag_SOURCES = testSipFrag.cxx
testSipMessage_SOURCES = testSipMessage.cxx
testSipMessageMemory_SOURCES = testSipMessageMemory.cxx
testSipStack1_SOURCES = testSipStack1.cxx
testStack_SOURCES = testStack.cxx
testTcp_SOURCES = testTcp.cxx
testThreadIf_SOURCES = testThreadIf.cxx
testTime_SOURCES = testTime.cxx
testTimer_SOURCES = testTimer.cxx
testTransactionFSM_SOURCES = testTransactionFSM.cxx
testTuple_SOURCES = testTuple.cxx
testTypedef_SOURCES = testTypedef.cxx
testUri_SOURCES = testUri.cxx
testXMLCursor_SOURCES = testXMLCursor.cxx

