# TODO - Do we need to actually install these files since reTurn is not a
# library?
SET(INCLUDES
   AsyncSocketBase.hxx
   AsyncSocketBaseHandler.hxx
   AsyncTcpSocketBase.hxx
   AsyncTlsSocketBase.hxx
   AsyncUdpSocketBase.hxx
   ChannelManager.hxx
   ConnectionManager.hxx
   DataBuffer.hxx
   ReTurnConfig.hxx
   ReTurnSubsystem.hxx
   RemotePeer.hxx
   RequestHandler.hxx
   StunAuth.hxx
   StunMessage.hxx
   StunTuple.hxx
   TcpConnection.hxx
   TcpServer.hxx
   TlsConnection.hxx
   TlsServer.hxx
   TurnAllocation.hxx
   TurnAllocationKey.hxx
   TurnAllocationManager.hxx
   TurnManager.hxx
   TurnPermission.hxx
   UdpRelayServer.hxx
   UdpServer.hxx
   UserAuthData.hxx
   reTurnServer.hxx
)

SET(SOURCES
   AsyncSocketBase.cxx
   AsyncTcpSocketBase.cxx
   AsyncTlsSocketBase.cxx
   AsyncUdpSocketBase.cxx
   ChannelManager.cxx
   ConnectionManager.cxx
   DataBuffer.cxx
   ReTurnConfig.cxx
   ReTurnSubsystem.cxx
   RemotePeer.cxx
   RequestHandler.cxx
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
   reTurnServer.cxx
)

INCLUDE_DIRECTORIES(..)

SET(TARGET_LIBS ${OPENSSL_LIBRARIES} rutil)

ADD_EXECUTABLE(reTurnServer ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(reTurnServer ${TARGET_LIBS})
SET_TARGET_PROPERTIES(reTurnServer PROPERTIES FOLDER reTurn)

INSTALL(TARGETS reTurnServer DESTINATION bin)

ADD_SUBDIRECTORY(client)
ADD_SUBDIRECTORY(test)
