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

# The Gstreamer webrtc component is still technically considered
# unstable.  Linking to it generates warnings at compile time.
# Setting the flag GST_USE_UNSTABLE_API disables the warnings.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGST_USE_UNSTABLE_API")

target_include_directories(echoTest PRIVATE ${GSTREAMERMM_1_0_INCLUDE_DIRS})
target_link_libraries(echoTest dum gstreamerutils)

install(TARGETS echoTest DESTINATION ${CMAKE_INSTALL_BINDIR})

