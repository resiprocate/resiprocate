
if(USE_KURENTO)
   add_subdirectory(kurento)
endif()
if(USE_GSTREAMER)
   add_subdirectory(gstreamer)
endif()
add_subdirectory(samples)

set(INCLUDES
   RTPPortManager.hxx
)

add_library(resipmedia
   RTPPortManager.cxx
   ${INCLUDES}
)

target_link_libraries(resipmedia PUBLIC rutil)

install(TARGETS resipmedia DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/media ${INCLUDES})

