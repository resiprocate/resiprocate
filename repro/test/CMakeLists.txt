function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER repro/Tests)
   target_link_libraries(${ARGV0} repro)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

#test(testDispatcher testDispatcher.cxx)

# Runs the server checks only for the backends given a server through the
# REPRO_TEST_MYSQL_* / REPRO_TEST_PGSQL_* environment variables (see the
# comment at the top of testSqlDb.cxx)
test_base(testSqlDb testSqlDb.cxx)
set_target_properties(testSqlDb PROPERTIES FOLDER repro/Tests)
target_link_libraries(testSqlDb reprolib)
if(WIN32)
   if(WITH_SSL)
      add_custom_command ( TARGET testSqlDb POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:testSqlDb>
        COMMAND_EXPAND_LISTS
      )
   endif()
   if(USE_BDB)
      add_custom_command ( TARGET testSqlDb POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:BERKELEYDB> $<TARGET_FILE_DIR:testSqlDb>
        COMMAND_EXPAND_LISTS
      )
   endif()
endif()
