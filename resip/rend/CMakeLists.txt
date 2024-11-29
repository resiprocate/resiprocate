
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

add_executable(rend
   rendIt.cxx
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

target_include_directories(rend PUBLIC ${ASIO_INCLUDE_DIRS})
target_include_directories(rend PUBLIC ${POPT_INCLUDE_DIRS})
target_include_directories(rend PUBLIC ${BOOST_INCLUDE_DIRS})
set_target_properties(rend PROPERTIES FOLDER resip/ManualTests)
target_link_libraries(rend PUBLIC dum ${POPT_LIBRARIES})

if(WIN32 AND WITH_SSL)
  add_custom_command ( TARGET rend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:rend>
    COMMAND_EXPAND_LISTS
  )
endif()


