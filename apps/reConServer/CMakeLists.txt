set(INCLUDES
   reConServer.hxx
   reConServerConfig.hxx
   AppSubsystem.hxx
   B2BCallManager.hxx
   CDRFile.hxx
   CredentialGrabber.hxx
   CredentialInfo.hxx
   MyConversationManager.hxx
   MyMessageDecorator.hxx
   MyUserAgent.hxx
   RegistrationForwarder.hxx
   SubscriptionForwarder.hxx
   ProtonCommandThread.hxx
)

add_executable(reConServer
   reConServer.cxx
   reConServerConfig.cxx
   AppSubsystem.cxx
   B2BCallManager.cxx
   CDRFile.cxx
   CredentialGrabber.cxx
   CredentialInfo.cxx
   MyConversationManager.cxx
   MyMessageDecorator.cxx
   MyUserAgent.cxx
   RegistrationForwarder.cxx
   SubscriptionForwarder.cxx
   ${INCLUDE}
)


target_add_conditional_sources(reConServer BUILD_QPID_PROTON
   ProtonCommandThread.cxx
)

target_include_directories(reConServer PRIVATE ${SOCI_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIR} ${MySQL_INCLUDE_DIRS})

target_link_libraries(reConServer PUBLIC recon reflow ${SOCI_LIBRARIES})

# FIXME
if(USE_SOCI_POSTGRESQL)
   target_link_libraries(reConServer PUBLIC -lsoci_postgresql)
endif()

if(USE_SOCI_MYSQL)
   target_link_libraries(reConServer PUBLIC -lsoci_mysql -lmysqlclient)
endif()

install(TARGETS reConServer DESTINATION ${CMAKE_INSTALL_SBINDIR})

