set(INCLUDES
   AddressTranslator.hxx
   AppSubsystem.hxx
   B2BSession.hxx
   ConfigParser.hxx
   IChatGatewayCmds.hxx
   IChatIPPortData.hxx
   IPCThread.hxx
   MediaRelay.hxx
   MediaRelayPort.hxx
   Server.hxx
   SipRegistration.hxx
   Thread.hxx
   Version.hxx
)

add_executable(ichat-gw
   AddressTranslator.cxx
   AppSubsystem.cxx
   B2BSession.cxx
   ConfigParser.cxx
   ichat-gw.cxx
   IChatIPPortData.cxx
   IPCThread.cxx
   MediaRelay.cxx
   MediaRelayPort.cxx
   Server.cxx
   SipRegistration.cxx
   Thread.cxx
   ${INCLUDES}
)

target_include_directories(ichat-gw PRIVATE ${PCRE_INCLUDE_DIRS})
target_link_libraries(ichat-gw PUBLIC dum ${PCRE_LIBRARIES})

install(TARGETS ichat-gw DESTINATION ${CMAKE_INSTALL_SBINDIR})

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/ichat-gw ${INCLUDES})

add_subdirectory(jabberconnector)
