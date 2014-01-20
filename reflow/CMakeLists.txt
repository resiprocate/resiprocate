# Used for installation
SET(INCLUDES
   ErrorCode.hxx
   FakeSelectSocketDescriptor.hxx
   Flow.hxx
   FlowDtlsSocketContext.hxx
   FlowDtlsTimerContext.hxx
   FlowManager.hxx
   FlowManagerException.hxx
   FlowManagerSubsystem.hxx
   MediaStream.hxx
)

# Special case because DTLS files are under "dtls_wrapper"
SET(INCLUDES_DTLS_FOLDER
   dtls_wrapper/bf_dwrap.h
   dtls_wrapper/DtlsFactory.hxx
   dtls_wrapper/DtlsSocket.hxx
   dtls_wrapper/DtlsTimer.hxx
)

# Used for compilation
SET(SOURCES
   FakeSelectSocketDescriptor.cxx
   Flow.cxx
   FlowDtlsSocketContext.cxx
   FlowDtlsTimerContext.cxx
   FlowManager.cxx
   FlowManagerSubsystem.cxx
   MediaStream.cxx
   dtls_wrapper/bf_dwrap.c
   dtls_wrapper/DtlsFactory.cxx
   dtls_wrapper/DtlsSocket.cxx
   dtls_wrapper/DtlsTimer.cxx
)

LIST(APPEND INCLUDES ${INCLUDES_DTLS_FOLDER})

INCLUDE_DIRECTORIES(..)
INCLUDE_DIRECTORIES(dtls_wrapper)

SET(TARGET_LIBS rutil reTurnClient)
#SET(TARGET_LIBS reTurn ${OPENSSL_LIBRARIES} ${ARES_LIBRARIES})

IF(CMAKE_HOST_WIN32)
   # Static library on Windows
   SET(TARGET_TYPE "")

#   LIST(APPEND TARGET_LIBS winmm)
ELSE()
   # Shared library on Unix
   SET(TARGET_TYPE SHARED)
   
   IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#      LIST(APPEND TARGET_LIBS rt)
   ENDIF()
ENDIF()

ADD_LIBRARY(reflow ${TARGET_TYPE} ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(reflow ${TARGET_LIBS})
SET_TARGET_PROPERTIES(reflow PROPERTIES FOLDER reflow)

INSTALL(TARGETS reflow DESTINATION lib)
INSTALL(FILES ${INCLUDES} DESTINATION include/reflow)
INSTALL(FILES ${INCLUDES_DTLS_FOLDER} DESTINATION include/reflow/dtls_wrapper)

ADD_SUBDIRECTORY(dtls_wrapper/test)
