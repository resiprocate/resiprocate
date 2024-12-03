function(sample_app)
   add_executable(${ARGV})
   target_include_directories(${ARGV0} PRIVATE ${POPT_INCLUDE_DIRS})
   target_link_libraries(${ARGV0} dum ${POPT_LIBRARIES})
   set_target_properties(${ARGV0} PROPERTIES FOLDER dum/SampleApps)
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
   set_target_properties(${ARGV0} PROPERTIES FOLDER dum/Tests)
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
endfunction()

set(BASICCLIENT_SRC
   basicClient.cxx
   basicClientCall.cxx
   basicClientCmdLineParser.cxx
   basicClientUserAgent.cxx
)

sample_app(basicClient ${BASICCLIENT_SRC})
sample_app(basicRegister basicRegister.cxx)
sample_app(basicMessage basicMessage.cxx)
#sample_app(certServer certServer.cxx)  # deprecated
sample_app(limpc-dum limpc.cxx UserAgent.cxx CommandLineParser.cxx)
#sample_app(rlsServer rlsServer.cxx)    # deprecated
#sample_app(test3pcc test3pcc.cxx)      # deprecated
test(BasicCall BasicCall.cxx)
test(testContactInstanceRecord testContactInstanceRecord.cxx)
test(testRequestValidationHandler testRequestValidationHandler.cxx)
test(testDialogSetId testDSI.cxx)
#test(testIdentity testIdentity.cxx)    # deprecated
test(testPubDocument testPubDocument.cxx)
test(testRedirectManager testRedirectManager.cxx)
#test(testSMIMEInvite testSMIMEInvite.cxx)   #deprecated
#test(testSMIMEMessage testSMIMEMessage.cxx) #deprecated

add_subdirectory(DialogInfoSubClient)
