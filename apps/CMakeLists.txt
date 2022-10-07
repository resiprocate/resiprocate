
add_subdirectory(clicktocall)
add_subdirectory(sipdial)
if(BUILD_ICHAT_GW)
   add_subdirectory(ichat-gw)
endif()
if(BUILD_TELEPATHY_CM)
   add_subdirectory(telepathy)
endif()
add_subdirectory(reConServer)
add_subdirectory(registration-agent)
add_subdirectory(echoTest)

