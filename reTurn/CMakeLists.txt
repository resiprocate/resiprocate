set(INCLUDES
    AsyncSocketBaseHandler.hxx
    AsyncSocketBase.hxx
    AsyncTcpSocketBase.hxx
    AsyncTlsSocketBase.hxx
    AsyncUdpSocketBase.hxx
    ChannelManager.hxx
    ConnectionManager.hxx
    DataBuffer.hxx
    RemotePeer.hxx
    RequestHandler.hxx
    ReTurnConfig.hxx
    ReTurnSubsystem.hxx
    StunAuth.hxx
    StunMessage.hxx
    StunTuple.hxx
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

add_executable(reTurnServer
    reTurnServer.cxx
    AsyncSocketBase.cxx
    AsyncUdpSocketBase.cxx
    AsyncTcpSocketBase.cxx
    AsyncTlsSocketBase.cxx
    ChannelManager.cxx
    ConnectionManager.cxx
    DataBuffer.cxx
    RemotePeer.cxx
    RequestHandler.cxx
    ReTurnConfig.cxx
    ReTurnSubsystem.cxx
    StunAuth.cxx
    StunMessage.cxx
    StunTuple.cxx
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

target_include_directories(reTurnServer PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(reTurnServer ${OPENSSL_LIBRARIES} rutil)
target_link_libraries(reTurnServer ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(reTurnServer PROPERTIES FOLDER reTurn)

install(TARGETS reTurnServer DESTINATION ${CMAKE_INSTALL_BINDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reTurn ${INCLUDES})

add_subdirectory(client)
add_subdirectory(test)
