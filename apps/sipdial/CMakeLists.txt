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
version_libname(sipdial)

add_executable(sipdialer
   sipdialer.cpp
)

target_link_libraries(sipdialer PUBLIC sipdial)

install(TARGETS sipdialer DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS sipdial DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/sipdial ${INCLUDES})

install(FILES sipdialer.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
