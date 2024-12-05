
set(INCLUDES
   ActiveCallInfo.hxx
   AppSubsystem.hxx
   ConfigParser.hxx
   HttpBase.hxx
   HttpConnection.hxx
   MOHManager.hxx
   ParkManager.hxx
   ParkOrbit.hxx
   Server.hxx
   WebAdmin.hxx
   WebAdminThread.hxx
)

add_executable(MOHParkServer
   AppSubsystem.cxx
   ConfigParser.cxx
   HttpBase.cxx
   HttpConnection.cxx
   MOHManager.cxx
   MOHParkServer.cxx
   ParkManager.cxx
   ParkOrbit.cxx
   Server.cxx
   WebAdmin.cxx
   WebAdminThread.cxx
   ${INCLUDES}
)

target_link_libraries(MOHParkServer PUBLIC recon)
if(WIN32)
  if(USE_SIPXTAPI)
     add_dependencies(MOHParkServer sipXtapi-pcre sipXportLib sipXsdpLib sipXmediaLib sipXmediaAdapterLib)
  endif()
  if(WITH_SSL)
    add_custom_command ( TARGET MOHParkServer POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:MOHParkServer>
      COMMAND_EXPAND_LISTS
    )
  endif()
endif()

install(TARGETS MOHParkServer DESTINATION ${CMAKE_INSTALL_SBINDIR})

set_target_properties(MOHParkServer PROPERTIES FOLDER recon/Apps)

