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
	Register.cxx \
	Registrar.cxx \
	TestSupport.cxx \
	test503Generator.cxx \
	testResponses.cxx \
	testRSP-2.cxx \
	testUdp.cxx \
	testServer.cxx \
	testClient.cxx \
	testDns.cxx \
	testMem.cxx \
	testRandomHex.cxx \
	testSelect.cxx \
	testSipStack1.cxx \
	testTransactionFSM.cxx \
	testTuple.cxx \
	testUri.cxx

#testTuple and testUri really belongs i check_PROGRAMS, but
#they broken for builds that don't specify USE_V6

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
# testSource testTlsConnection testTransportSelector
# testTypes Transceiver

LDADD = libtestsupport.a ../libresiprocate.la
check_PROGRAMS = \
	SipTortureTests \
	testApplicationSip \
	testAppTimer \
	testCoders \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDigestAuthentication \
	testDnsUtil \
	testEmbedded \
	testEmptyHeader \
	testExternalLogger \
	testFifo \
	testFileSystem \
	testIM \
	testIntrusiveList \
	testLogger \
	testMessageWaiting \
	testMultipartMixedContents \
	testMultipartRelated \
	testParseBuffer \
	testParserCategories \
	testPidf \
	testPksc7 \
	testPlainContents \
	testRlmi \
	testSHA1Stream \
	testSdp \
	testSelectInterruptor \
	testSipFrag \
	testSipMessage \
	testSipMessageMemory \
	testStack \
	testTcp \
	testThreadIf \
	testTime \
	testTimer \
	testTypedef \
	testXMLCursor 

bin_PROGRAMS = limpc testTcp testDataPerformance

TESTS = $(check_PROGRAMS)
SipTortureTests_SOURCES = SipTortureTests.cxx
limpc_SOURCES = limpc.cxx
testApplicationSip_SOURCES = testApplicationSip.cxx
testAppTimer_SOURCES = testAppTimer.cxx
testCoders_SOURCES = testCoders.cxx
testCountStream_SOURCES = testCountStream.cxx
testData_SOURCES = testData.cxx
testDataPerformance_SOURCES = testDataPerformance.cxx
testDataStream_SOURCES = testDataStream.cxx
testDigestAuthentication_SOURCES = testDigestAuthentication.cxx
testDnsUtil_SOURCES = testDnsUtil.cxx
testEmbedded_SOURCES = testEmbedded.cxx
testEmptyHeader_SOURCES = testEmptyHeader.cxx
testExternalLogger_SOURCES = testExternalLogger.cxx
testFifo_SOURCES = 	testFifo.cxx
testFileSystem_SOURCES = testFileSystem.cxx
testIM_SOURCES = testIM.cxx
testIntrusiveList_SOURCES = testIntrusiveList.cxx
testLogger_SOURCES = testLogger.cxx
testMessageWaiting_SOURCES = testMessageWaiting.cxx
testMultipartMixedContents_SOURCES = testMultipartMixedContents.cxx
testMultipartRelated_SOURCES = testMultipartRelated.cxx
testParseBuffer_SOURCES = testParseBuffer.cxx
testParserCategories_SOURCES = testParserCategories.cxx
testPidf_SOURCES = testPidf.cxx
testPksc7_SOURCES = testPksc7.cxx
testPlainContents_SOURCES = testPlainContents.cxx
testRlmi_SOURCES = testRlmi.cxx
testSHA1Stream_SOURCES = testSHA1Stream.cxx
testSdp_SOURCES = testSdp.cxx
testSelectInterruptor_SOURCES = testSelectInterruptor.cxx
testSipFrag_SOURCES = testSipFrag.cxx
testSipMessage_SOURCES = testSipMessage.cxx
testSipMessageMemory_SOURCES = testSipMessageMemory.cxx
testStack_SOURCES = testStack.cxx
testTcp_SOURCES = testTcp.cxx
testThreadIf_SOURCES = testThreadIf.cxx
testTime_SOURCES = testTime.cxx
testTimer_SOURCES = testTimer.cxx
testTypedef_SOURCES = testTypedef.cxx
testXMLCursor_SOURCES = testXMLCursor.cxx

