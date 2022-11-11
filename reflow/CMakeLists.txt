set(INCLUDES
   ErrorCode.hxx
   FakeSelectSocketDescriptor.hxx
   FlowDtlsSocketContext.hxx
   FlowDtlsTimerContext.hxx
   Flow.hxx
   FlowContext.hxx
   FlowManagerException.hxx
   FlowManager.hxx
   FlowManagerSubsystem.hxx
   HEPRTCPEventLoggingHandler.hxx
   MediaStream.hxx
   RTCPEventLoggingHandler.hxx
   Srtp2Helper.hxx
   dtls_wrapper/bf_dwrap.hxx
   dtls_wrapper/DtlsFactory.hxx
   dtls_wrapper/DtlsSocket.hxx
   dtls_wrapper/DtlsTimer.hxx
)

add_library(reflow
   FakeSelectSocketDescriptor.cxx
   Flow.cxx
   FlowDtlsSocketContext.cxx
   FlowDtlsTimerContext.cxx
   FlowManager.cxx
   FlowManagerSubsystem.cxx
   HEPRTCPEventLoggingHandler.cxx
   MediaStream.cxx
   dtls_wrapper/DtlsTimer.cxx
   dtls_wrapper/DtlsSocket.cxx
   dtls_wrapper/DtlsFactory.cxx
   dtls_wrapper/bf_dwrap.cxx

   ${INCLUDES}
)

target_include_directories(reflow PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/reflow/dtls_wrapper>)
target_include_directories(reflow PUBLIC ${SRTP2_INCLUDE_DIRS})
target_link_libraries(reflow reTurnClient ${SRTP2_LIBRARIES})
set_target_properties(reflow PROPERTIES FOLDER reflow)
version_libname(reflow)

install(TARGETS reflow DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reflow ${INCLUDES})

add_subdirectory(dtls_wrapper/test)
