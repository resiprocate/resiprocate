function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER tfm/repro)
   target_link_libraries(${ARGV0} tfdum)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
   if(WIN32 AND USE_BDB)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:BERKELEYDB> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
  endif()
endfunction()

set(INCLUDES
   BindHandle.hxx
   ClientOutOfDialogReqEvent.hxx
   ClientPagerMessageEvent.hxx
   ClientPublicationEvent.hxx
   ClientRegistrationEvent.hxx
   ClientSubscriptionEvent.hxx
   DialogEventExpect.hxx
   DialogEventHandlerEvent.hxx
   DumEvent.hxx
   DumExpect.hxx
   DumFixture.hxx
   DumUaAction.hxx
   DumUserAgent.hxx
   InviteSessionEvent.hxx
   ServerOutOfDialogReqEvent.hxx
   ServerPagerMessageEvent.hxx
   ServerSubscriptionEvent.hxx
   TestClientPagerMessage.hxx
   TestClientPublication.hxx
   TestClientRegistration.hxx
   TestClientSubscription.hxx
   TestDialogEvent.hxx
   TestInviteSession.hxx
   TestServerOutOfDialogReq.hxx
   TestServerPagerMessage.hxx
   TestServerSubscription.hxx
   TestUsage.hxx
   TestUserAgent.hxx
   TfdTcpTransport.hxx
   TfdTestSipEndPoint.hxx
)

add_library(tfdum
   DumUserAgent.cxx
   TestDialogEvent.cxx
   DialogEventExpect.cxx
   DumUaAction.cxx
   DumEvent.cxx
   DumExpect.cxx
   DumFixture.cxx
   TestClientSubscription.cxx
   TestServerSubscription.cxx
   TestClientRegistration.cxx
   TestUsage.cxx
   TestInviteSession.cxx
   TestClientPublication.cxx
   TestServerOutOfDialogReq.cxx
   TestClientPagerMessage.cxx
   TestServerPagerMessage.cxx
   TfdTcpTransport.cxx
   TfdTestSipEndPoint.cxx
   ${INCLUDES}
)

target_link_libraries(tfdum PUBLIC tfmrepro)

if(BUILD_TESTING)
    test(TestLamba TestLamba.cxx)
    test(dumTests dumTests.cxx)
endif()

# For windows - put all TFM executables into a central install location to make it easier to run
if(WIN32)
    install(TARGETS dumTests DESTINATION tfm)
endif()
