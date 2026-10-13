function(copy_openssl_dlls)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

function(manual_test)
   add_executable(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/ManualTests)
   target_link_libraries(${ARGV0} reflow)
   copy_openssl_dlls(${ARGV0})
endfunction()

function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   target_link_libraries(${ARGV0} reflow)
   copy_openssl_dlls(${ARGV0})
endfunction()

set(SHARED_SRC
   CreateCert.cxx
   TestDtlsUdp.cxx
   TestTimerContext.cxx
)

if(WITH_SSL)
   test(testDtlsWrapper testDtlsWrapper.cxx CreateCert.cxx)
   # testDtlsServer <port> and testDtlsClient <host> <port> exchange DTLS-SRTP over
   # UDP; lines typed into the client are echoed back upper-cased by the server
   manual_test(testDtlsClient testDtlsClient.cxx ${SHARED_SRC})
   manual_test(testDtlsServer testDtlsServer.cxx ${SHARED_SRC})
endif()
