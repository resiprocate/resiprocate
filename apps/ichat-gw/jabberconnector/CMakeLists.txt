
# FIXME - ../*.cxx

set(INCLUDES
   IChatUser.hxx
   JabberComponent.hxx
   JabberUserDb.hxx
)

add_executable(ichat-gw-jc
   ichat-gw-jc.cxx
   IChatUser.cxx
   ../IPCThread.cxx
   JabberComponent.cxx
   JabberUserDb.cxx
   ../Thread.cxx
   ${INCLUDES}
)

install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/ichat-gw-jc ${INCLUDES})
