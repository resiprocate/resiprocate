
set(INCLUDES
   PyRouteWorker.hxx
   PyThreadSupport.hxx
   PyRouteProcessor.hxx
)

add_library(pyroute
   PyRoutePlugin.cxx
   PyRouteWorker.cxx
   PyRouteProcessor.cxx
   cxxextensions.c
   cxx_extensions.cxx
   cxxsupport.cxx
   IndirectPythonInterface.cxx
   cxx_exceptions.cxx
   ${INCLUDES}
)

target_include_directories(pyroute PUBLIC ${Python3_INCLUDE_DIRS} ${PYCXX_INCLUDE_DIRS})

target_link_libraries(pyroute PUBLIC ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})
