function(manual_test)
   add_executable(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} dum ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/Tests)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

function(test)
   test_base(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} dum ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER resip/Tests)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

set(SHARED_SRC
   CommandLineParser.cxx
   RegEventClient.cxx
   UserAgent.cxx
   basicClientCall.cxx
   basicClientCmdLineParser.cxx
   basicClientUserAgent.cxx
)

manual_test(basicRegister basicRegister.cxx ${SHARED_SRC})
manual_test(BasicCall BasicCall.cxx ${SHARED_SRC})
manual_test(basicMessage basicMessage.cxx ${SHARED_SRC})
manual_test(basicClient basicClient.cxx ${SHARED_SRC})
test(testContactInstanceRecord testContactInstanceRecord.cxx)
test(testRequestValidationHandler testRequestValidationHandler.cxx ${SHARED_SRC})

add_subdirectory(DialogInfoSubClient)
