CXX = ${DISTCC} @CXX@
INCLUDES = -I$(top_srcdir) -I$(top_builddir)
noinst_LIBRARIES = libtestsupport.a
libtestsupport_a_SOURCES = \
	InviteClient.cxx \
	InviteServer.cxx \
	Register.cxx \
	Registrar.cxx \
	TestSupport.cxx 

noinst_HEADERS = \
	digcalc.hxx \
	md5.hxx \
	Resolver.hxx \
	InviteClient.hxx \
	Register.hxx \
	testIM.hxx \
	InviteServer.hxx \
	Registrar.hxx \
	TestSupport.hxx

LDADD = libtestsupport.a ../libresiprocate.a
check_PROGRAMS = \
	testSource \
	SipTortureTests \
	lg \
	test1 \
	test2 \
	testApplicationSip \
	testClient \
	testCoders \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDns \
	testDigestAuthentication \
	testEmbedded \
	testEmptyHeader \
	testIM \
	testLogger \
	testMessageWaiting \
	testMultipartMixedContents \
	testParseBuffer \
	testParserCategories \
	testPlainContents \
	testPreparse \
	testRandomHex \
	testSdp \
	testServer \
	testSipFrag \
	testSipMessage \
	testSipMessageMemory \
	testSpeed \
	testThreadIf \
	testTimer \
	testTransactionFSM \
	testUri \
	testXMLCursor \
	testUdp \
	testTcp \
	testStack

bin_PROGRAMS = limpc testTcp

TESTS = $(check_PROGRAMS)
SipTortureTests_SOURCES = SipTortureTests.cxx
lg_SOURCES = lg.cxx
#testDnsResolver_SOURCES = testDnsResolver.cxx
#testLockStep_SOURCES = testLockStep.cxx
#testSipStack1_SOURCES = testSipStack1.cxx
limpc_SOURCES = limpc.cxx
test1_SOURCES = test1.cxx
test2_SOURCES = test2.cxx
testApplicationSip_SOURCES = testApplicationSip.cxx
testClient_SOURCES = testClient.cxx
testCoders_SOURCES = testCoders.cxx
testCountStream_SOURCES = testCountStream.cxx
testDataPerformance_SOURCES = testDataPerformance.cxx
testDataStream_SOURCES = testDataStream.cxx
testData_SOURCES = testData.cxx
testDigestAuthentication_SOURCES = testDigestAuthentication.cxx
testDns_SOURCES = testDns.cxx
testEmbedded_SOURCES = testEmbedded.cxx
testEmptyHeader_SOURCES = testEmptyHeader.cxx
testIM_SOURCES = testIM.cxx
testLogger_SOURCES = testLogger.cxx
testMessageWaiting_SOURCES = testMessageWaiting.cxx
testMultipartMixedContents_SOURCES = testMultipartMixedContents.cxx
testParseBuffer_SOURCES = testParseBuffer.cxx
testParserCategories_SOURCES = testParserCategories.cxx
testPlainContents_SOURCES = testPlainContents.cxx
testPreparse_SOURCES = testPreparse.cxx
testRandomHex_SOURCES = testRandomHex.cxx
testSdp_SOURCES = testSdp.cxx
testServer_SOURCES = testServer.cxx
testSipFrag_SOURCES = testSipFrag.cxx
testSipMessageMemory_SOURCES = testSipMessageMemory.cxx
testSipMessage_SOURCES = testSipMessage.cxx
testSource_SOURCES = testSource.cxx
testSpeed_SOURCES = testSpeed.cxx
testStack_SOURCES = testStack.cxx
testTcp_SOURCES = testTcp.cxx
testThreadIf_SOURCES = testThreadIf.cxx
testTimer_SOURCES = testTimer.cxx
testTransactionFSM_SOURCES = testTransactionFSM.cxx
testUdp_SOURCES = testUdp.cxx
testUri_SOURCES = testUri.cxx
testXMLCursor_SOURCES = testXMLCursor.cxx


