
configure_file(
   org.freedesktop.Telepathy.ConnectionManager.resiprocate.service.in
   org.freedesktop.Telepathy.ConnectionManager.resiprocate.service
   @ONLY)

install(FILES
   org.freedesktop.Telepathy.ConnectionManager.resiprocate.service
   DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)

