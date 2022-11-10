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

target_include_directories(echoTest PRIVATE ${GSTREAMERMM_1_0_INCLUDE_DIRS})
target_link_libraries(echoTest dum gstreamerutils)

install(TARGETS echoTest DESTINATION ${CMAKE_INSTALL_BINDIR})

