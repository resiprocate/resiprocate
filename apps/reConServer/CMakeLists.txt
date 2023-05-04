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
   PyConversationManager.hxx
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
   ${INCLUDES}
)


target_add_conditional_sources(reConServer BUILD_QPID_PROTON
   ProtonCommandThread.cxx
)

target_add_conditional_sources(reConServer BUILD_PYTHON
   PyConversationManager.cxx
)

target_include_directories(reConServer PRIVATE ${SOCI_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIR} ${MySQL_INCLUDE_DIRS})

if(BUILD_PYTHON)
   target_include_directories(reConServer PRIVATE ${Python3_INCLUDE_DIRS} ${PYCXX_INCLUDE_DIRS})
endif()

target_link_libraries(reConServer PUBLIC recon ${SOCI_LIBRARIES})

# FIXME
if(USE_SOCI_POSTGRESQL)
   target_link_libraries(reConServer PUBLIC -lsoci_postgresql)
endif()

if(USE_SOCI_MYSQL)
   target_link_libraries(reConServer PUBLIC -lsoci_mysql ${MySQL_LIBRARIES})
endif()

if(BUILD_PYTHON)
   target_link_libraries(reConServer PUBLIC ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})
endif()

install(TARGETS reConServer DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES reConServer.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8)
