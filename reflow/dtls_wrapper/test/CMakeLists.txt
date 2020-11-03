function(manual_test)
   add_executable(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   target_link_libraries(${ARGV0} reflow rutil ${OPENSSL_LIBRARIES} ${SRTP_LIBRARIES})
endfunction()

function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   target_link_libraries(${ARGV0} reflow rutil ${OPENSSL_LIBRARIES} ${SRTP_LIBRARIES})
endfunction()

set(SHARED_SRC
   CreateCert.cxx
   TestDtlsUdp.cxx
   TestTimerContext.cxx
)

test(testDtlsWrapper testDtlsWrapper.cxx ${SHARED_SRC})
manual_test(testDtlsClient testDtlsClient.cxx ${SHARED_SRC})
manual_test(testDtlsServer testDtlsServer.cxx ${SHARED_SRC})
