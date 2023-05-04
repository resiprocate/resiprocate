set(INCLUDES_COMMON
   AsyncSocketBase.hxx
   AsyncTcpSocketBase.hxx
   AsyncTlsSocketBase.hxx
   AsyncUdpSocketBase.hxx
   ChannelManager.hxx
   DataBuffer.hxx
   RemotePeer.hxx
   ReTurnSubsystem.hxx
   StunMessage.hxx
   StunTuple.hxx
)

set(INCLUDES
   AsyncSocketBaseHandler.hxx
   ConnectionManager.hxx
   RequestHandler.hxx
   ReTurnConfig.hxx
   StunAuth.hxx
   TcpConnection.hxx
   TcpServer.hxx
   TlsConnection.hxx
   TlsServer.hxx
   TurnAllocation.hxx
   TurnAllocationManager.hxx
   TurnAllocationKey.hxx
   TurnManager.hxx
   TurnPermission.hxx
   UdpRelayServer.hxx
   UdpServer.hxx
   UserAuthData.hxx
   reTurnServer.hxx
)

add_library(reTurnCommon
   AsyncSocketBase.cxx
   AsyncTcpSocketBase.cxx
   AsyncTlsSocketBase.cxx
   AsyncUdpSocketBase.cxx
   ChannelManager.cxx
   DataBuffer.cxx
   RemotePeer.cxx
   ReTurnSubsystem.cxx
   StunMessage.cxx
   StunTuple.cxx
   ${INCLUDES_COMMON}
)

add_executable(reTurnServer
   reTurnServer.cxx
   ConnectionManager.cxx
   RequestHandler.cxx
   ReTurnConfig.cxx
   StunAuth.cxx
   TcpConnection.cxx
   TcpServer.cxx
   TlsConnection.cxx
   TlsServer.cxx
   TurnAllocation.cxx
   TurnAllocationKey.cxx
   TurnAllocationManager.cxx
   TurnManager.cxx
   TurnPermission.cxx
   UdpRelayServer.cxx
   UdpServer.cxx
   UserAuthData.cxx

   ${INCLUDES}
)

target_include_directories(reTurnCommon PUBLIC ${ASIO_INCLUDE_DIRS})
target_include_directories(reTurnCommon PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(reTurnCommon rutil)
set_target_properties(reTurnCommon PROPERTIES FOLDER reTurn)
version_libname(reTurnCommon)

target_include_directories(reTurnServer PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(reTurnServer reTurnCommon)
set_target_properties(reTurnServer PROPERTIES FOLDER reTurn)

install(TARGETS reTurnCommon DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS reTurnServer DESTINATION ${CMAKE_INSTALL_SBINDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reTurn ${INCLUDES_COMMON} ${INCLUDES})
install(FILES psql-user-extract DESTINATION ${INSTALL_RETURN_PKGLIB_DIR})
install(FILES reTurnServer.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8)

add_subdirectory(client)
add_subdirectory(test)
