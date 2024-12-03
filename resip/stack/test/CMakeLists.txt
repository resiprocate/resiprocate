function(manual_test)
   add_executable(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} resip ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/ManualTests)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

function(test)
   test_base(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} resip ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/Tests)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

manual_test(UAS UAS.cxx)
test(testEmptyHfv testEmptyHfv.cxx)
test(RFC4475TortureTests RFC4475TortureTests.cxx)
set(TORTURETEST_DATS
   ${CMAKE_SOURCE_DIR}/resip/stack/test/badaspec.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/badbranch.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/baddate.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/baddn.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/badinv01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/badvers.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/bcast.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/bext01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/bigcode.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/clerr.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/cparam01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/cparam02.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/dblreq.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/esc01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/esc02.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/escnull.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/escruri.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/insuf.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/intmeth.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/inv2543.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/invut.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/longreq.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/ltgtruri.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/lwsdisp.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/lwsruri.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/lwsstart.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/mcl01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/mismatch01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/mismatch02.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/mpart01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/multi01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/ncl.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/noreason.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/novelsc.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/quotbal.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/regaut01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/regbadct.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/regescrt.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/scalar02.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/scalarlg.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/sdp01.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/semiuri.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/test.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/transports.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/trws.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/unkscm.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/unksm2.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/unreason.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/wsinv.dat
   ${CMAKE_SOURCE_DIR}/resip/stack/test/zeromf.dat
)
add_custom_command ( TARGET RFC4475TortureTests POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
   ${TORTURETEST_DATS} ${CMAKE_BINARY_DIR}/resip/stack/test
   COMMAND_EXPAND_LISTS
)
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
test(testTypedef testTypedef.cxx)
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
