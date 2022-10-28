set(INCLUDES
   AppSubsystem.hxx
   EchoTestConfig.hxx
)

add_executable(echoTest
   echoTest.cxx
   AppSubsystem.cxx
   EchoTestConfig.cxx
   ${INCLUDES}
)

# FIXME - gstreamer
target_include_directories(echoTest PRIVATE ${GSTREAMERMM_INCLUDE_DIRS})
target_link_libraries(echoTest dum gstreamerutils)

install(TARGETS echoTest DESTINATION ${CMAKE_INSTALL_BINDIR})

