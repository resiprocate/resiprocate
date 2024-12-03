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

set(CONFIG_FILES
   ${CMAKE_SOURCE_DIR}/reTurn/reTurnServer.config
   ${CMAKE_SOURCE_DIR}/reTurn/users.txt
   ${CMAKE_SOURCE_DIR}/reTurn/server.key
   ${CMAKE_SOURCE_DIR}/reTurn/server.pem
   ${CMAKE_SOURCE_DIR}/reTurn/dh2048.pem
)

add_custom_command ( TARGET reTurnServer POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
   ${CONFIG_FILES} ${CMAKE_BINARY_DIR}/reTurn
   COMMAND_EXPAND_LISTS
)

target_include_directories(reTurnServer PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(reTurnServer reTurnCommon)
set_target_properties(reTurnServer PROPERTIES FOLDER reTurn)

if(WIN32 AND WITH_SSL)
  add_custom_command ( TARGET reTurnServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:reTurnServer>
    COMMAND_EXPAND_LISTS
  )
endif()

install(TARGETS reTurnCommon DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS reTurnServer DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(FILES reTurnServer.config DESTINATION ${CMAKE_INSTALL_SBINDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reTurn ${INCLUDES_COMMON} ${INCLUDES})
install(FILES psql-user-extract DESTINATION ${INSTALL_RETURN_PKGLIB_DIR})
install(FILES reTurnServer.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8)

add_subdirectory(client)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
