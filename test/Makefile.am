CXX = ${DISTCC} @CXX@
LIBTOOL = @LIBTOOL@ --tag=CXX
INCLUDES = -I$(top_srcdir) -I$(top_builddir)
noinst_LTLIBRARIES = libtestsupport.la
libtestsupport_la_SOURCES = \
	InviteClient.cxx \
	InviteServer.cxx \
	Register.cxx \
	Registrar.cxx \
	Resolver.cxx \
	TestSupport.cxx \
	Transceiver.cxx

noinst_HEADERS = \
	digcalc.hxx \
	md5.hxx \
	Resolver.hxx \
	InviteClient.hxx \
	Transceiver.hxx \
	Register.hxx \
	testIM.hxx \
	InviteServer.hxx \
	Registrar.hxx \
	TestSupport.hxx

LDADD = libtestsupport.la ../libresiprocate.la
check_PROGRAMS = \
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
	testTcpTransport \
	testThreadIf \
	testTimer \
	testTransactionFSM \
	testUri \
	testXMLCursor

TESTS = $(check_PROGRAMS)
SipTortureTests_SOURCES = SipTortureTests.cxx
lg_SOURCES = lg.cxx
test1_SOURCES = test1.cxx
test2_SOURCES = test2.cxx
testApplicationSip_SOURCES = testApplicationSip.cxx
testClient_SOURCES = testClient.cxx
testCoders_SOURCES = testCoders.cxx
testCountStream_SOURCES = testCountStream.cxx
testData_SOURCES = testData.cxx
testDataPerformance_SOURCES = testDataPerformance.cxx
testDataStream_SOURCES = testDataStream.cxx
testDigestAuthentication_SOURCES = testDigestAuthentication.cxx
#testDnsResolver_SOURCES = testDnsResolver.cxx
testEmbedded_SOURCES = testEmbedded.cxx
testEmptyHeader_SOURCES = testEmptyHeader.cxx
testIM_SOURCES = testIM.cxx
#testLockStep_SOURCES = testLockStep.cxx
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
testSipMessage_SOURCES = testSipMessage.cxx
testSipMessageMemory_SOURCES = testSipMessageMemory.cxx
#testSipStack1_SOURCES = testSipStack1.cxx
testSpeed_SOURCES = testSpeed.cxx
testTcpTransport_SOURCES = testTcpTransport.cxx
testThreadIf_SOURCES = testThreadIf.cxx
testTimer_SOURCES = testTimer.cxx
testTransactionFSM_SOURCES = testTransactionFSM.cxx
testUri_SOURCES = testUri.cxx
testXMLCursor_SOURCES = testXMLCursor.cxx
