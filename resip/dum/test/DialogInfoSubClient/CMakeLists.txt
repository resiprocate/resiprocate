function(manual_test)
   add_executable(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} dum ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER dum/SampleApps)
endfunction()

set(SRC
   Call.cxx
   CmdLineParser.cxx
   DialogInfoSubClient.cxx
   UserAgent.cxx
)

manual_test(DialogInfoSubClient ${SRC})

if(WIN32 AND WITH_SSL)
  add_custom_command ( TARGET DialogInfoSubClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:DialogInfoSubClient>
    COMMAND_EXPAND_LISTS
  )
endif()