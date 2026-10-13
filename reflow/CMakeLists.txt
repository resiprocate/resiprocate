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

   ${INCLUDES}
)

target_include_directories(reflow PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/reflow/dtls_wrapper>)
target_include_directories(reflow PUBLIC ${SRTP2_INCLUDE_DIRS})
if(WIN32 AND USE_CONTRIB)
  target_link_libraries(reflow reTurnClient srtp2)
else()
  target_link_libraries(reflow reTurnClient ${SRTP2_LIBRARIES})
endif()
set_target_properties(reflow PROPERTIES FOLDER reflow)
version_libname(reflow)

install(TARGETS reflow DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/reflow ${INCLUDES})

if(BUILD_TESTING)
   # For the test directories: test() is run by ctest, manual_test() isn't
   function(copy_openssl_dlls)
      if(WIN32 AND WITH_SSL)
        add_custom_command ( TARGET ${ARGV0} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
          COMMAND_EXPAND_LISTS
        )
      endif()
   endfunction()

   function(manual_test)
      add_executable(${ARGV})
      set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/ManualTests)
      target_link_libraries(${ARGV0} reflow)
      copy_openssl_dlls(${ARGV0})
   endfunction()

   function(test)
      test_base(${ARGV})
      set_target_properties(${ARGV0} PROPERTIES FOLDER reflow/Tests)
      target_link_libraries(${ARGV0} reflow)
      copy_openssl_dlls(${ARGV0})
   endfunction()

   add_subdirectory(dtls_wrapper/test)
   add_subdirectory(test)
endif()
