
set(INCLUDES
   PyRouteWorker.hxx
   PyRouteMessageHandler.hxx
   PyRouteProcessor.hxx
)

add_library(pyroute MODULE
   PyRoutePlugin.cxx
   PyRouteWorker.cxx
   PyRouteMessageHandler.cxx
   PyRouteProcessor.cxx
   ${INCLUDES}
)

target_include_directories(pyroute PUBLIC ${Python3_INCLUDE_DIRS} ${PYCXX_INCLUDE_DIRS})

target_link_libraries(pyroute PUBLIC reprolib ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})

install(TARGETS pyroute DESTINATION ${INSTALL_REPRO_PLUGIN_DIR})
