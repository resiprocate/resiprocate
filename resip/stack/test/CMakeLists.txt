function(manual_test)
   add_executable(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} resip ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/ManualTests)
endfunction()

function(test)
   test_base(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} resip ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/Tests)
endfunction()

manual_test(UAS UAS.cxx)
manual_test(testEmptyHfv testEmptyHfv.cxx)
manual_test(RFC4475TortureTests RFC4475TortureTests.cxx)
manual_test(limpc limpc.cxx)
test(testAppTimer testAppTimer.cxx)
test(testApplicationSip testApplicationSip.cxx TestSupport.cxx)
manual_test(testClient testClient.cxx)
test(testConnectionBase testConnectionBase.cxx TestSupport.cxx)
test(testCorruption testCorruption.cxx)
test(testDialogInfoContents testDialogInfoContents.cxx TestSupport.cxx)
test(testDigestAuthentication testDigestAuthentication.cxx TestSupport.cxx)
manual_test(testDtlsTransport testDtlsTransport.cxx)
test(testEmbedded testEmbedded.cxx TestSupport.cxx)
test(testEmptyHeader testEmptyHeader.cxx TestSupport.cxx)
test(testExternalLogger testExternalLogger.cxx)
test(testGenericPidfContents testGenericPidfContents.cxx TestSupport.cxx)
test(testIM testIM.cxx)
manual_test(testLockStep testLockStep.cxx)
test(testMessageWaiting testMessageWaiting.cxx)
test(testMultipartMixedContents testMultipartMixedContents.cxx TestSupport.cxx)
test(testMultipartRelated testMultipartRelated.cxx TestSupport.cxx)
test(testParserCategories testParserCategories.cxx)
test(testPidf testPidf.cxx)
test(testPksc7 testPksc7.cxx TestSupport.cxx)
test(testPlainContents testPlainContents.cxx)
manual_test(testResponses testResponses.cxx)
test(testRlmi testRlmi.cxx TestSupport.cxx)
test(testDtmfPayload testDtmfPayload.cxx)
test(testSdp testSdp.cxx TestSupport.cxx)
manual_test(testSelect testSelect.cxx)
test(testSelectInterruptor testSelectInterruptor.cxx)
manual_test(testServer testServer.cxx)
test(testSipFrag testSipFrag.cxx TestSupport.cxx)
test(testSipMessage testSipMessage.cxx TestSupport.cxx)
manual_test(testSipMessageEncode testSipMessageEncode.cxx)
test(testSipMessageMemory testSipMessageMemory.cxx TestSupport.cxx)
manual_test(testSipStack1 testSipStack1.cxx)
if(NOT WIN32)
manual_test(testSipStackNetNs testSipStackNetNs.cxx)
endif()
test(testStack testStack.cxx)
test(testTcp testTcp.cxx)
test(testTime testTime.cxx)
test(testTimer testTimer.cxx)
if(NOT WIN32)
manual_test(testTransactionFSM testTransactionFSM.cxx TestSupport.cxx)
endif()
test(testTransportSelector testTransportSelector.cxx)
test(testTuple testTuple.cxx)
manual_test(testTypedef testTypedef.cxx)
manual_test(testUdp testUdp.cxx)
test(testUri testUri.cxx TestSupport.cxx)
test(testWsCookieContext testWsCookieContext.cxx)

if(OPENSSL_FOUND)
   test(testSocketFunc testSocketFunc.cxx)
   test(testSecurity testSecurity.cxx)
endif()

# fuzzing targets

add_fuzzing_executable(fuzzStack fuzzStack.cxx TestSupport.cxx)
set_target_properties(fuzzStack PROPERTIES FOLDER resip/Fuzzing)
target_include_directories(fuzzStack PRIVATE ${POPT_INCLUDE_DIRS})
target_link_libraries(fuzzStack PRIVATE resip ${POPT_LIBRARIES})
