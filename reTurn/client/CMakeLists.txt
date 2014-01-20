# TODO: Do we need to install libreTurnClient?
SET(INCLUDES
   ../AsyncSocketBase.hxx
   ../AsyncTcpSocketBase.hxx
   ../AsyncTlsSocketBase.hxx
   ../AsyncUdpSocketBase.hxx
   ../ChannelManager.hxx
   ../DataBuffer.hxx
   ../RemotePeer.hxx
   ../ReTurnSubsystem.hxx
   ../StunMessage.hxx
   ../StunTuple.hxx
   ErrorCode.hxx
   TurnAsyncSocket.hxx
   TurnAsyncSocketHandler.hxx
   TurnAsyncTcpSocket.hxx
   TurnAsyncTlsSocket.hxx
   TurnAsyncUdpSocket.hxx
   TurnSocket.hxx
   TurnTcpSocket.hxx
   TurnTlsSocket.hxx
   TurnUdpSocket.hxx
)

SET(SOURCES
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
)

INCLUDE_DIRECTORIES(..)

SET(TARGET_LIBS "")

IF(CMAKE_HOST_WIN32)
   # Static library on Windows
   SET(TARGET_TYPE "")
   #LIST(APPEND TARGET_LIBS winmm)
ELSE()
   # Shared library on Unix
   SET(TARGET_TYPE SHARED)
   
   #IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   #   LIST(APPEND TARGET_LIBS rt)
   #ENDIF()
ENDIF()

ADD_LIBRARY(reTurnClient ${TARGET_TYPE} ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(reTurnClient ${TARGET_LIBS})
SET_TARGET_PROPERTIES(reTurnClient PROPERTIES FOLDER reTurn)

ADD_SUBDIRECTORY(test)
