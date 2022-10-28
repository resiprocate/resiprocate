
set(INCLUDES
   GStreamerUtils.hxx
   GstRtpManager.hxx
   GstRtpSession.hxx
   GstSubsystem.hxx
)

add_library(gstreamerutils
   GStreamerUtils.cxx
   GstRtpManager.cxx
   GstRtpSession.cxx
   GstSubsystem.cxx
   ${INCLUDES}
)

target_include_directories(gstreamerutils PUBLIC ${GSTREAMERMM_INCLUDE_DIRS} ${GST_WEBRTC_INCLUDE_DIRS})
target_link_libraries(gstreamerutils PUBLIC resip resipmedia ${GSTREAMERMM_LIBRARIES} ${GST_WEBRTC_LIBRARIES})
version_libname(gstreamerutils)

install(TARGETS gstreamerutils DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/media/gstreamer ${INCLUDES})
