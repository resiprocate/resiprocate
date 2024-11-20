add_executable(repro
   repro.cxx
   HttpBase.cxx
   HttpConnection.cxx
   ReproRunner.cxx
   ReproVersion.cxx
   WebAdmin.cxx
   WebAdminThread.cxx
   )

target_link_libraries(repro PUBLIC reprolib)

install(TARGETS repro DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(FILES users.txt repro.config DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(FILES doc/repro.8 doc/reprocmd.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8)

set(INCLUDES
   AbstractDb.hxx
   AccountingCollector.hxx
   Ack200DoneMessage.hxx
   AclStore.hxx
   AsyncProcessor.hxx
   AsyncProcessorMessage.hxx
   AsyncProcessorWorker.hxx
   AuthenticatorFactory.hxx
   BerkeleyDb.hxx
   BasicWsConnectionValidator.hxx
   CommandServer.hxx
   CommandServerThread.hxx
   ConfigStore.hxx
   FilterStore.hxx
   ForkControlMessage.hxx
   HttpBase.hxx
   HttpConnection.hxx
   monkeys/AmIResponsible.hxx
   monkeys/CertificateAuthenticator.hxx
   monkeys/CookieAuthenticator.hxx
   monkeys/ConstantLocationMonkey.hxx
   monkeys/DigestAuthenticator.hxx
   monkeys/GruuMonkey.hxx
   monkeys/IsTrustedNode.hxx
   monkeys/LocationServer.hxx
   monkeys/OutboundTargetHandler.hxx
   monkeys/QValueTargetHandler.hxx
   monkeys/RADIUSAuthenticator.hxx
   monkeys/RecursiveRedirect.hxx
   monkeys/SimpleStaticRoute.hxx
   monkeys/SimpleTargetHandler.hxx
   monkeys/StaticRoute.hxx
   monkeys/StrictRouteFixup.hxx
   monkeys/GeoProximityTargetSorter.hxx
   monkeys/RequestFilter.hxx
   monkeys/MessageSilo.hxx
   MySqlDb.hxx
   OutboundTarget.hxx
   PersistentMessageQueue.hxx
   Plugin.hxx
   PostgreSqlDb.hxx
   ProcessorChain.hxx
   Processor.hxx
   ProcessorMessage.hxx
   Proxy.hxx
   ProxyConfig.hxx
   QValueTarget.hxx
   Registrar.hxx
   RegSyncClient.hxx
   RegSyncServer.hxx
   RegSyncServerThread.hxx
   ReproAuthenticatorFactory.hxx
   ReproRADIUSServerAuthManager.hxx
   ReproServerAuthManager.hxx
   ReproTlsPeerAuthManager.hxx
   ReproRunner.hxx
   ReproVersion.hxx
   RequestContext.hxx
   ResponseContext.hxx
   RouteStore.hxx
   RRDecorator.hxx
   SiloStore.hxx
   SqlDb.hxx
   stateAgents/CertPublicationHandler.hxx
   stateAgents/CertServer.hxx
   stateAgents/CertSubscriptionHandler.hxx
   stateAgents/PresencePublicationHandler.hxx
   stateAgents/PresenceServer.hxx
   stateAgents/PresenceSubscriptionHandler.hxx
   stateAgents/PrivateKeyPublicationHandler.hxx
   stateAgents/PrivateKeySubscriptionHandler.hxx
   StaticRegStore.hxx
   Store.hxx
   Target.hxx
   TimerCMessage.hxx
   TlsPeerIdentityInfo.hxx
   TlsPeerIdentityStore.hxx
   UserAuthGrabber.hxx
   UserInfoMessage.hxx
   UserStore.hxx
   WebAdmin.hxx
   WebAdminThread.hxx
   XmlRpcConnection.hxx
   XmlRpcServerBase.hxx
)

add_library(reprolib
   BerkeleyDb.cxx
   RouteStore.cxx
   UserStore.cxx
   ConfigStore.cxx
   AclStore.cxx
   StaticRegStore.cxx
   FilterStore.cxx
   SiloStore.cxx
   Store.cxx
   AbstractDb.cxx
   BasicWsConnectionValidator.cxx

   CommandServer.cxx
   CommandServerThread.cxx
   ProxyConfig.cxx

   AccountingCollector.cxx
   Proxy.cxx
   Registrar.cxx
   RegSyncClient.cxx
   RegSyncServer.cxx
   RegSyncServerThread.cxx
   ReproAuthenticatorFactory.cxx
   ReproRADIUSServerAuthManager.cxx
   ReproServerAuthManager.cxx
   ReproTlsPeerAuthManager.cxx
   RequestContext.cxx
   ResponseContext.cxx
   RRDecorator.cxx
   SqlDb.cxx
   Processor.cxx
   ProcessorChain.cxx
   Target.cxx
   TlsPeerIdentityStore.cxx
   UserAuthGrabber.cxx
   XmlRpcConnection.cxx
   XmlRpcServerBase.cxx
   OutboundTarget.cxx
   PersistentMessageQueue.cxx
   QValueTarget.cxx

   stateAgents/PresenceServer.cxx
   stateAgents/PresencePublicationHandler.cxx
   stateAgents/PresenceSubscriptionHandler.cxx
   monkeys/CertificateAuthenticator.cxx
   monkeys/DigestAuthenticator.cxx
   monkeys/CookieAuthenticator.cxx
   monkeys/StrictRouteFixup.cxx
   monkeys/AmIResponsible.cxx
   monkeys/IsTrustedNode.cxx
   monkeys/ConstantLocationMonkey.cxx
   monkeys/LocationServer.cxx
   monkeys/OutboundTargetHandler.cxx
   monkeys/RADIUSAuthenticator.cxx
   monkeys/RecursiveRedirect.cxx
   monkeys/SimpleStaticRoute.cxx
   monkeys/StaticRoute.cxx
   monkeys/QValueTargetHandler.cxx
   monkeys/SimpleTargetHandler.cxx
   monkeys/GeoProximityTargetSorter.cxx
   monkeys/RequestFilter.cxx
   monkeys/MessageSilo.cxx

   ${INCLUDES}
)

target_add_conditional_sources(reprolib OPENSSL_FOUND
   stateAgents/CertServer.cxx
   stateAgents/CertPublicationHandler.cxx
   stateAgents/CertSubscriptionHandler.cxx
   stateAgents/PrivateKeyPublicationHandler.cxx
   stateAgents/PrivateKeySubscriptionHandler.cxx
)

target_add_conditional_sources(reprolib BUILD_DSO_PLUGINS
   Plugin.cxx
)

target_link_libraries(reprolib PUBLIC dum)
if(BUILD_DSO_PLUGINS)
   target_link_libraries(reprolib PUBLIC ${CMAKE_DL_LIBS})
endif()

target_include_directories(reprolib PUBLIC ${BERKELEYDB_INCLUDE_DIRS})
target_link_libraries(reprolib PUBLIC ${BERKELEYDB_LIBRARIES})

target_include_directories(reprolib PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(reprolib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(reprolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(reprolib PUBLIC ${CAJUN_INCLUDE_DIRS})
target_include_directories(reprolib PUBLIC ${PCRE_INCLUDE_DIRS})
target_link_libraries(reprolib PUBLIC ${PCRE_LIBRARIES})
if(USE_MAXMIND_GEOIP)
   target_include_directories(reprolib PUBLIC ${GEOIP_INCLUDE_DIRS})
   target_link_libraries(reprolib PUBLIC ${GEOIP_LIBRARIES})
endif()

target_add_conditional_sources(reprolib MySQL_FOUND
   MySqlDb.cxx
   )
target_link_libraries(reprolib PUBLIC ${MySQL_LIBRARIES})
target_include_directories(reprolib PUBLIC ${MySQL_INCLUDE_DIRS})

target_add_conditional_sources(reprolib USE_POSTGRESQL
   PostgreSqlDb.cxx
   )

if(USE_POSTGRESQL)
   # for some reason we can't use PostgreSQL_LIBRARIES here...
   target_link_libraries(reprolib PUBLIC PostgreSQL::PostgreSQL)
endif()

version_libname(reprolib)

target_include_directories(repro PUBLIC ${PostgreSQL_INCLUDE_DIR})

install(TARGETS reprolib DESTINATION ${CMAKE_INSTALL_LIBDIR})

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/repro ${INCLUDES})

if(BUILD_DSO_PLUGINS)
   add_subdirectory(plugins)
endif()

add_subdirectory(accountingconsumers)
add_subdirectory(reprocmd)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
