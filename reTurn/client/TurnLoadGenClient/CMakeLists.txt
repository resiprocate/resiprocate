function(sample_app)
   add_executable(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reTurn/client/SampleApps)
   target_link_libraries(${ARGV0} reTurnClient)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

sample_app(TurnLoadGenClient TurnLoadGenClient.cxx TurnLoadGenAsyncSocketHandler.cxx)

