set(INCLUDES
   ConfigParser.hxx
   Version.hxx
   WebAdminThread.hxx
   XmlRpcServerBase.hxx
   HttpBase.hxx
   XmlRpcConnection.hxx
   AppSubsystem.hxx
   XmlRpcServerThread.hxx
   B2BSession.hxx
   AddressTranslator.hxx
   WebAdmin.hxx
   XmlRpcServer.hxx
   HttpConnection.hxx
   Server.hxx
   ClickToCallCmds.hxx
)

add_executable(clicktocall
   AddressTranslator.cxx
   AppSubsystem.cxx
   B2BSession.cxx
   clicktocall.cxx
   ConfigParser.cxx
   HttpBase.cxx
   HttpConnection.cxx
   Server.cxx
   WebAdmin.cxx
   WebAdminThread.cxx
   XmlRpcConnection.cxx
   XmlRpcServerBase.cxx
   XmlRpcServer.cxx
   XmlRpcServerThread.cxx
   ${INCLUDES}
)

target_include_directories(clicktocall PRIVATE ${PCRE_INCLUDE_DIRS})
target_link_libraries(clicktocall PUBLIC dum ${PCRE_LIBRARIES})

install(TARGETS clicktocall DESTINATION ${CMAKE_INSTALL_SBINDIR})

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/clicktocall ${INCLUDES})
