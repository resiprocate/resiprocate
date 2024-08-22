set(INCLUDES
   Continuation.hxx
   Event.hxx
   KurentoConnection.hxx
   KurentoException.hxx
   KurentoManager.hxx
   KurentoResponseHandler.hxx
   KurentoSubsystem.hxx
   Object.hxx
)

add_library(kurentoclient
   Event.cxx
   KurentoConnection.cxx
   KurentoManager.cxx
   KurentoSubsystem.cxx
   Object.cxx
   ${INCLUDES}
)

target_include_directories(kurentoclient PUBLIC ${ASIO_INCLUDE_DIRS})
target_include_directories(kurentoclient PUBLIC ${WEBSOCKETPP_INCLUDE_DIRS})
target_include_directories(kurentoclient PUBLIC ${CAJUN_INCLUDE_DIRS})
target_link_libraries(kurentoclient rutil)
set_target_properties(kurentoclient PROPERTIES FOLDER kurentoclient)
version_libname(kurentoclient)

install(TARGETS kurentoclient DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/media/kurento ${INCLUDES})

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
