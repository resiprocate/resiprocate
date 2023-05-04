
set(INCLUDES
   RendAcct.hxx
   RendDlg.hxx
   RendDlgAcct.hxx
   RendKa.hxx
   RendMisc.hxx
   RendReport.hxx
   RendRunner.hxx
   RendSketch.hxx
   RendTroop.hxx
   RendWavePres.hxx
)

add_library(rend
   RendAcct.cxx
   RendDlg.cxx
   RendDlgAcct.cxx
   RendKa.cxx
   RendMisc.cxx
   RendPres.cxx
   RendReg.cxx
   RendRunner.cxx
   RendSketch.cxx
   RendTroop.cxx
   ${INCLUDES}
)

add_executable(rendIt
   rendIt.cxx
)

target_include_directories(rend PUBLIC ${ASIO_INCLUDE_DIRS})
target_include_directories(rend PUBLIC ${POPT_INCLUDE_DIRS})
target_link_libraries(rend PUBLIC dum ${POPT_LIBRARIES})
version_libname(rend)

target_link_libraries(rendIt PUBLIC rend)

install(TARGETS rend DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS rendIt DESTINATION ${CMAKE_INSTALL_BINDIR})

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/rend ${INCLUDES})
