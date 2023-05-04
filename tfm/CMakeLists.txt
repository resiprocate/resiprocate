
add_subdirectory(repro)

add_subdirectory(tfdum)

set(INCLUDES
   ActionBase.hxx
   AsciiGraphic.hxx
   BindPointerAction.hxx
   Box.hxx
   CheckContacts.hxx
   CheckFetchedContacts.hxx
   CheckPrivacy.hxx
   CommonAction.hxx
   CountDown.hxx
   CppTestSelector.hxx
   CPTextTestProgressListener.hxx
   DialogSet.hxx
   DnsUtils.hxx
   EndPoint.hxx
   Event.hxx
   ExpectActionEvent.hxx
   ExpectAction.hxx
   ExpectFunctorDefs.hxx
   Expect.hxx
   PortAllocator.hxx
   predicates/ExpectUtils.hxx
   predicates/NotifyContents.hxx
   predicates/SubscribeExpired.hxx
   predicates/SubscribeExpires.hxx
   Renderer.hxx
   Resolver.hxx
   RouteGuard.hxx
   RtpEvent.hxx
   SeqDecls.hxx
   SeqDefns.hxx
   SequenceClassConstructorDecls.hxx
   SequenceClassConstructorDefns.hxx
   Sequence.hxx
   SequenceSet.hxx
   SequenceSetThread.hxx
   SipEvent.hxx
   SipRawMessage.hxx
   Source.hxx
   StunEndPoint.hxx
   StunEvent.hxx
   StunServer.hxx
   SubDecls.hxx
   SubDefns.hxx
   TelnetClient.hxx
   TestEndPoint.hxx
   TestException.hxx
   TestProxy.hxx
   TestRtp.hxx
   TestSipElement.hxx
   TestSipEndPoint.hxx
   TestUser.hxx
   TfmDns.hxx
   TfmHelper.hxx
   TransportDriver.hxx
   WrapperEvent.hxx
)

add_library(tfm
   ActionBase.cxx 
   CppTestSelector.cxx 
   StunEndPoint.cxx 
   StunServer.cxx 
   EndPoint.cxx 
   TfmHelper.cxx 
   Expect.cxx 
   StunEvent.cxx 
   TfmDns.cxx 
   CPTestProgressListener.cxx 
   TestRtp.cxx 
   RtpEvent.cxx 
   CheckPrivacy.cxx 
   AsciiGraphic.cxx 
   Box.cxx 
   CheckContacts.cxx
   CheckFetchedContacts.cxx 
   CountDown.cxx 
   DialogSet.cxx 
   DnsUtils.cxx 
   Event.cxx 
   ExpectAction.cxx 
   ExpectActionEvent.cxx 
   Renderer.cxx 
   Resolver.cxx 
   RouteGuard.cxx 
   Sequence.cxx 
   SequenceSet.cxx 
   SequenceSetThread.cxx 
   SipEvent.cxx 
   SipRawMessage.cxx 
   Source.cxx 
   TelnetClient.cxx 
   TestEndPoint.cxx 
   TestException.cxx 
   TestProxy.cxx 
   TestSipEndPoint.cxx 
   TestUser.cxx 
   TransportDriver.cxx 
   predicates/ExpectUtils.cxx 
   predicates/SubscribeExpired.cxx 
   predicates/SubscribeExpires.cxx 
   predicates/NotifyContents.cxx
   ${INCLUDES}
)

target_include_directories(tfm PUBLIC )
target_link_libraries(tfm dum ${Netxx_LIBRARIES} ${CPPUNIT_LIBRARIES})
set_target_properties(tfm PROPERTIES FOLDER tfm)
version_libname(tfm)

install(TARGETS tfm DESTINATION ${CMAKE_INSTALL_LIBDIR})

