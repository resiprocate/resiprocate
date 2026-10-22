set(INCLUDES
   AppSubsystem.hxx
   CommandThread.hxx
   KeyedFile.hxx
   RegConfig.hxx
   SNMP_reSIProcate.hxx
   SNMPThread.hxx
   UserAccount.hxx
   UserRegistrationClient.hxx
)

add_executable(registrationAgent
   AppSubsystem.cxx
   CommandThread.cxx
   KeyedFile.cxx
   RegConfig.cxx
   registrationAgent.cxx
   SNMP_reSIProcate.cxx
   SNMPThread.cxx
   UserAccount.cxx
   UserRegistrationClient.cxx
)

target_include_directories(registrationAgent PRIVATE ${NETSNMP_AGENT_INCLUDE_DIRS})
target_link_libraries(registrationAgent PUBLIC dum ${NETSNMP_AGENT_LIBRARIES})

install(TARGETS registrationAgent DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES REG-AGENT-MIB.txt DESTINATION ${CMAKE_INSTALL_MIBDIR})

install(FILES registrationAgent.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8)
