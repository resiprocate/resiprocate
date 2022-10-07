file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/reproInfo.hxx" [=[#include "config.h"]=])

add_executable(repro-bin
        repro.cxx
        )

target_link_libraries(repro-bin PRIVATE repro)

set_target_properties(repro-bin PROPERTIES OUTPUT_NAME repro)

install(TARGETS repro-bin DESTINATION ${CMAKE_INSTALL_SBINDIR})

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
   reproInfo.hxx
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

add_library(repro
   RouteStore.cxx
   UserStore.cxx
   ConfigStore.cxx
   AclStore.cxx
   StaticRegStore.cxx
   FilterStore.cxx
   SiloStore.cxx
   Store.cxx
   AbstractDb.cxx
   BerkeleyDb.cxx
   BasicWsConnectionValidator.cxx

   CommandServer.cxx
   CommandServerThread.cxx
   ProxyConfig.cxx
   ReproVersion.cxx
   HttpBase.cxx
   HttpConnection.cxx
   WebAdmin.cxx
   WebAdminThread.cxx

   AccountingCollector.cxx
   Proxy.cxx
   Registrar.cxx
   RegSyncClient.cxx
   RegSyncServer.cxx
   RegSyncServerThread.cxx
   ReproRunner.cxx
   ReproAuthenticatorFactory.cxx
   ReproRADIUSServerAuthManager.cxx
   ReproServerAuthManager.cxx
   ReproTlsPeerAuthManager.cxx
   RequestContext.cxx
   ResponseContext.cxx
   RRDecorator.cxx
   Processor.cxx
   ProcessorChain.cxx
   SqlDb.cxx
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

target_add_conditional_sources(repro OPENSSL_FOUND
   stateAgents/CertServer.cxx
   stateAgents/CertPublicationHandler.cxx
   stateAgents/CertSubscriptionHandler.cxx
   stateAgents/PrivateKeyPublicationHandler.cxx
   stateAgents/PrivateKeySubscriptionHandler.cxx
)

target_link_libraries(repro PUBLIC dum)

target_include_directories(repro PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(repro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(repro PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(repro PRIVATE ${CAJUN_INCLUDE_DIRS})
target_include_directories(repro PRIVATE ${BERKELEYDB_INCLUDE_DIRS})
target_link_libraries(repro PRIVATE ${BERKELEYDB_LIBRARIES})
if(GEOIP_FOUND)
        target_link_libraries(repro PRIVATE ${GEOIP_LIBRARIES})
endif()

target_add_conditional_sources(repro MySQL_FOUND
        MySqlDb.cxx
        )
target_link_libraries(repro PRIVATE ${MySQL_LIBRARIES})
target_include_directories(repro PRIVATE ${MySQL_INCLUDE_DIRS})

target_add_conditional_sources(repro PostgreSQL_FOUND
        PostgreSqlDb.cxx
        )
target_link_libraries(repro PRIVATE ${PostgreSQL_LIBRARIES})
target_include_directories(repro PRIVATE ${PostgreSQL_INCLUDE_DIRS})

if(BUILD_REPRO_DSO_PLUGINS)
   add_subdirectory(plugins)
endif()
add_subdirectory(accountingconsumers)
add_subdirectory(reprocmd)
add_subdirectory(test)
