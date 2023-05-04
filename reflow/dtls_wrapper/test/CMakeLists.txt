function(manual_test)
   add_executable(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   target_link_libraries(${ARGV0} reflow)
endfunction()

function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   target_link_libraries(${ARGV0} reflow)
endfunction()

set(SHARED_SRC
   CreateCert.cxx
   TestDtlsUdp.cxx
   TestTimerContext.cxx
)

if(USE_SRTP1)
# srtp_priv.h isn't shipped in the packages for srtp2
# see: https://www.resiprocate.org/bugzilla/show_bug.cgi?id=169
# Therefore, we don't try to build the test at present.
   test(testDtlsWrapper testDtlsWrapper.cxx ${SHARED_SRC})
   manual_test(testDtlsClient testDtlsClient.cxx ${SHARED_SRC})
   manual_test(testDtlsServer testDtlsServer.cxx ${SHARED_SRC})
endif()
