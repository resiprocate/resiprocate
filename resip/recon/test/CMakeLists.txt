function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/recon/Tests)
   target_link_libraries(${ARGV0} recon)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

test(sdpTests sdpTests.cxx)
test(unitTests unitTests.cxx)

add_executable(testUA
   testUA.cxx
)

target_link_libraries(testUA recon)

install(TARGETS testUA DESTINATION ${CMAKE_INSTALL_BINDIR})

