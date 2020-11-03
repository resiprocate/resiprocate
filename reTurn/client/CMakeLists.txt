set(INCLUDES
   ErrorCode.hxx
   TurnAsyncSocketHandler.hxx
   TurnAsyncSocket.hxx
   TurnAsyncTcpSocket.hxx
   TurnAsyncTlsSocket.hxx
   TurnAsyncUdpSocket.hxx
   TurnSocket.hxx
   TurnTcpSocket.hxx
   TurnTlsSocket.hxx
   TurnUdpSocket.hxx
)

add_library(reTurnClient
   ../AsyncSocketBase.cxx
   ../AsyncTcpSocketBase.cxx
   ../AsyncTlsSocketBase.cxx
   ../AsyncUdpSocketBase.cxx
   ../ChannelManager.cxx
   ../DataBuffer.cxx
   ../RemotePeer.cxx
   ../ReTurnSubsystem.cxx
   ../StunMessage.cxx
   ../StunTuple.cxx
   TurnAsyncSocket.cxx
   TurnAsyncSocketHandler.cxx
   TurnAsyncTcpSocket.cxx
   TurnAsyncTlsSocket.cxx
   TurnAsyncUdpSocket.cxx
   TurnSocket.cxx
   TurnTcpSocket.cxx
   TurnTlsSocket.cxx
   TurnUdpSocket.cxx

   ${INCLUDES}
)

target_include_directories(reTurnClient PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/reTurn>)
target_link_libraries(reTurnClient ${OPENSSL_LIBRARIES} rutil)
set_target_properties(reTurnClient PROPERTIES FOLDER reTurn)
set_target_properties(reTurnClient PROPERTIES VERSION ${SO_RELEASE})

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reTurn/client ${INCLUDES})

add_subdirectory(test)
