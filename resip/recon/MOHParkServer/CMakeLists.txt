
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
