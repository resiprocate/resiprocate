set(INCLUDES
   Common.hxx
   Connection.hxx
   TextChannel.hxx
   MyConversationManager.hxx
   MyMessageDecorator.hxx
   MyUserAgent.hxx
   MyInstantMessage.hxx
   SipCallChannel.hxx
   TelepathyConversationProfile.hxx
   TelepathyMasterProfile.hxx
   TelepathyParameters.hxx
   Protocol.hxx
)

add_executable(telepathy-resiprocate
   Common.cxx
   Connection.cxx
   TextChannel.cxx
   MyConversationManager.cxx
   MyMessageDecorator.cxx
   MyUserAgent.cxx
   MyInstantMessage.cxx
   Protocol.cxx
   SipCallChannel.cxx
   TelepathyConversationProfile.cxx
   TelepathyMasterProfile.cxx
   TelepathyParameters.cxx
   telepathy-resiprocate.cxx
   ${INCLUDES}
)

target_include_directories(telepathy-resiprocate PRIVATE ${TELEPATHY_QT5_INCLUDE_DIR})
target_include_directories(telepathy-resiprocate PRIVATE ${Qt5Core_INCLUDE_DIRS} ${Qt5DBus_INCLUDE_DIRS})
target_include_directories(telepathy-resiprocate PRIVATE ${SIPXTAPI_INCLUDE_DIRS})

target_link_libraries(telepathy-resiprocate PUBLIC recon)

install(TARGETS telepathy-resiprocate DESTINATION ${CMAKE_INSTALL_BINDIR})

