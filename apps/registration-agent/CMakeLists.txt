
cmake_minimum_required(VERSION 2.8)
set(APP_SRCS registrationAgent.cxx AppSubsystem.cxx CommandThread.cxx KeyedFile.cxx RegConfig.cxx UserAccount.cxx UserRegistrationClient.cxx)
add_executable(registrationAgent ${APP_SRCS})
target_link_libraries(registrationAgent rutil resip dum qpid-proton-cpp)

