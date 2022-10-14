
set(INCLUDES
   PyRouteWorker.hxx
   PyRouteProcessor.hxx
)

add_library(pyroute
   PyRoutePlugin.cxx
   PyRouteWorker.cxx
   PyRouteProcessor.cxx
   ${INCLUDES}
)

target_include_directories(pyroute PUBLIC ${Python3_INCLUDE_DIRS} ${PYCXX_INCLUDE_DIRS})

target_link_libraries(pyroute PUBLIC ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})

install(TARGETS pyroute DESTINATION ${INSTALL_REPRO_PLUGIN_DIR})
