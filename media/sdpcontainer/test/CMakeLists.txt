function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resipmedia/Tests)
   target_link_libraries(${ARGV0} resipmedia)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

test(testSdpHelper testSdpHelper.cxx)

