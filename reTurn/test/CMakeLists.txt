function(TEST)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reTurn/Tests)
   target_link_libraries(${ARGV0} reTurnClient)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

test(stunTestVectors stunTestVectors.cxx)
if(WITH_SSL)
   test(testTlsSockets testTlsSockets.cxx TlsTestSupport.hxx)
endif()

# reTurnServer end to end over UDP, TCP and TLS, run in-process with the sync and async clients
test(testReTurnServer testReTurnServer.cxx TlsTestSupport.hxx)
target_link_libraries(testReTurnServer reTurnServerLib)
target_compile_definitions(testReTurnServer PRIVATE RETURN_TEST_DH_FILE="${CMAKE_SOURCE_DIR}/reTurn/dh2048.pem")
