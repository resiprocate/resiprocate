set(INCLUDES
   DialerConfiguration.hxx
   DialInstance.hxx
   MyInviteSessionHandler.hxx
)

add_library(sipdial
   DialInstance.cpp
   MyInviteSessionHandler.cpp
   DialerConfiguration.cpp
   ${INCLUDES}
)

target_link_libraries(sipdial PUBLIC dum)

add_executable(sipdialer
   sipdialer.cpp
)

target_link_libraries(sipdialer PUBLIC sipdial)

install(TARGETS sipdialer DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS sipdial DESTINATION ${CMAKE_INSTALL_LIBDIR})


