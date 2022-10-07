function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/recon/Tests)
   target_link_libraries(${ARGV0} recon)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

test(sdpTests sdpTests.cxx)
test(unitTests unitTests.cxx)

# FIXME playback_prompt.h record_prompt.h

add_executable(testUA
   testUA.cxx
   playback_prompt.h
   record_prompt.h
)

target_link_libraries(testUA recon)
