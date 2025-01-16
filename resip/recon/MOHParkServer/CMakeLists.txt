
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

set(CONFIG_FILES
   ${CMAKE_SOURCE_DIR}/resip/recon/MOHParkServer/MOHParkServer.config
)

add_custom_command ( TARGET MOHParkServer POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
   ${CONFIG_FILES} ${CMAKE_BINARY_DIR}/resip/recon/MOHParkServer
   COMMAND_EXPAND_LISTS
)

target_link_libraries(MOHParkServer PUBLIC recon)
if(WIN32)
   if(USE_SIPXTAPI)
     if(SIPXTAPI_PROJS_IN_VS_GUI)
       add_dependencies(MOHParkServer ${SIPXTAPI_VSPROJ_DEPS})
     endif()
     add_custom_command ( TARGET MOHParkServer POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:SIPXTAPI_CODEC_PLUGINS> $<TARGET_FILE_DIR:MOHParkServer>
       COMMAND_EXPAND_LISTS
     )
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

