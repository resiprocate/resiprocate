INCLUDE_DIRECTORIES(../..)

FUNCTION(TEST)
   TEST_BASE(${ARGV})
   IF(CMAKE_HOST_WIN32)
      TARGET_LINK_LIBRARIES(${ARGV0} rutil resiprocate)
   ELSE()
      TARGET_LINK_LIBRARIES(${ARGV0} rutil resip ${POPT_LIBRARIES})
   ENDIF()
   SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES FOLDER resip/Tests)   
ENDFUNCTION()

# TODO: libpopt
# TODO: Python
# TODO: A lot of files still depend on config.h, have CMake generate it if
# still needed
# TODO: Only enable tests that are already enabled by default... Some are not even compiling

#TEST(testClient testClient.cxx InviteClient.cxx Resolver.cxx)
TEST(testParserCategories testParserCategories.cxx)
TEST(testAor testAor.cxx)
TEST(testCorruption testCorruption.cxx)
TEST(testDns testDns.cxx)
#TEST(testDnsCache testDnsCache.cxx)
#TEST(testDnsStub testDnsStub.cxx)
TEST(testAppTimer testAppTimer.cxx)
TEST(testApplicationSip testApplicationSip.cxx TestSupport.cxx)
TEST(testConnectionBase testConnectionBase.cxx TestSupport.cxx)
TEST(testDigestAuthentication testDigestAuthentication.cxx TestSupport.cxx)
TEST(testEmptyHeader testEmptyHeader.cxx TestSupport.cxx)
TEST(testMultipartMixedContents testMultipartMixedContents.cxx TestSupport.cxx)
TEST(testMultipartRelated testMultipartRelated.cxx TestSupport.cxx)
TEST(testPksc7 testPksc7.cxx TestSupport.cxx)
TEST(testRlmi testRlmi.cxx TestSupport.cxx)
TEST(testSdp testSdp.cxx TestSupport.cxx)
TEST(testSipFrag testSipFrag.cxx TestSupport.cxx)
TEST(testSipMessage testSipMessage.cxx TestSupport.cxx)
TEST(testSipMessageEncode testSipMessageEncode.cxx)
TEST(testSipMessageMemory testSipMessageMemory.cxx TestSupport.cxx)
TEST(testTransactionFSM testTransactionFSM.cxx TestSupport.cxx)
TEST(testUri testUri.cxx TestSupport.cxx)
TEST(testUdp testUdp.cxx)
TEST(testEmbedded testEmbedded.cxx)
TEST(test503Generator test503Generator.cxx)
#TEST(testBlast testBlast.cxx)
TEST(testClient testClient.cxx)
TEST(testConvertRaw testConvertRaw.cxx)
#TEST(testDigestAuthentication2 testDigestAuthentication2.cxx)
TEST(testDtlsTransport testDtlsTransport.cxx)
TEST(testEmptyHfv testEmptyHfv.cxx)
TEST(testExternalBodyContents testExternalBodyContents.cxx TestSupport.cxx)
TEST(testExternalLogger testExternalLogger.cxx)
#TEST(testForce testForce.cxx)
#TEST(testGperfHash testGperfHash.cxx)
#TEST(testHack testHack.cxx)
TEST(testHash testHash.cxx)
#TEST(testHashCasen testHashCasen.cxx)
#TEST(testHeaderFieldValueList testHeaderFieldValueList.cxx)
TEST(testIM testIM.cxx)
#TEST(testId testId.cxx)
TEST(testIdentity testIdentity.cxx TestSupport.cxx)
TEST(testLockStep testLockStep.cxx)
TEST(testMessageWaiting testMessageWaiting.cxx)
#TEST(testMsgHeaderDiagram testMsgHeaderDiagram.cxx)
#TEST(testNameAddrParamExclusions testNameAddrParamExclusions.cxx)
#TEST(testNonInviteClientTx testNonInviteClientTx.cxx)
#TEST(testNonInviteServerTx testNonInviteServerTx.cxx)
TEST(testParseNameAddr testParseNameAddr.cxx)
#TEST(testParseUtil testParseUtil.cxx)
TEST(testPidf testPidf.cxx)
TEST(testPlainContents testPlainContents.cxx)
TEST(testRSP-2 testRSP-2.cxx)
TEST(testResponses testResponses.cxx)
TEST(testSelect testSelect.cxx)
TEST(testSelectInterruptor testSelectInterruptor.cxx)
TEST(testServer testServer.cxx)
TEST(testSipStack1 testSipStack1.cxx)
#TEST(testSipStackInvite testSipStackInvite.cxx)
#TEST(testSource testSource.cxx)
TEST(testStack testStack.cxx)
#TEST(testStun testStun.cxx)
TEST(testTcp testTcp.cxx)
TEST(testTime testTime.cxx)
TEST(testTimer testTimer.cxx)
TEST(testTls testTls.cxx)
#TEST(testTlsConnection testTlsConnection.cxx)
#TEST(testTransportSelector testTransportSelector.cxx)
TEST(testTuple testTuple.cxx)
TEST(testTypedef testTypedef.cxx)
#TEST(testTypes testTypes.cxx)
TEST(testWsCookieContext testWsCookieContext.cxx)

IF(${OPENSSL_FOUND})
   TEST(testSMIME testSMIME.cxx)
   TEST(testSecurity testSecurity.cxx)
   TEST(testSocketFunc testSocketFunc.cxx)
   
   IF(CMAKE_HOST_APPLE)
      TEST(testMacSecurity testMacSecurity.cxx)
   ENDIF()
ENDIF()

#InviteClient.cxx
#InviteServer.cxx
#RFC4475TortureTests.cxx
#Register.cxx
#Registrar.cxx
#Resolver.cxx
#SipTortureTests.cxx
#TestSupport.cxx
#Transceiver.cxx
#UAS.cxx
#dumpTls.cxx
#lg.cxx
#limpc.cxx
#makeSelfCert.cxx
#md5.cxx
