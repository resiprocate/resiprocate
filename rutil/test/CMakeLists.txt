function(extra_test)
   add_executable(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER rutil/ExtraTests)
   target_link_libraries(${ARGV0} rutil)
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
   set_target_properties(${ARGV0} PROPERTIES FOLDER rutil/Tests)
   target_link_libraries(${ARGV0} rutil)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

test(testCompat testCompat.cxx)
test(testCoders testCoders.cxx)
test(testCondition testCondition.cxx)
test(testConfigParse testConfigParse.cxx)
test(testCountStream testCountStream.cxx)
test(testData testData.cxx)
test(testDataPerformance testDataPerformance.cxx)
test(testDataStream testDataStream.cxx)
test(testDnsUtil testDnsUtil.cxx)
test(testFifo testFifo.cxx)
test(testFileSystem testFileSystem.cxx)
test(testInserter testInserter.cxx)
test(testIntrusiveList testIntrusiveList.cxx)
test(testLogger TestSubsystemLogLevel.cxx TestSubsystemLogLevel.hxx testLogger.cxx)
test(testMD5Stream testMD5Stream.cxx)
if(RTC_OS_UNIX)
  test(testNetNs testNetNs.cxx)
endif()
test(testParseBuffer testParseBuffer.cxx)
test(testRandomHex testRandomHex.cxx)
test(testRandomThread testRandomThread.cxx)
test(testSHA1Stream testSHA1Stream.cxx)
test(testThreadIf testThreadIf.cxx)
test(testXMLCursor testXMLCursor.cxx)

# unreferenced in Makefile.am

extra_test(testKeyValueStore testKeyValueStore.cxx)

# fuzzing targets

add_fuzzing_executable(fuzzUtil fuzzUtil.cxx)
set_target_properties(fuzzUtil PROPERTIES FOLDER rutil/FuzzingTests)
target_link_libraries(fuzzUtil PRIVATE rutil)

