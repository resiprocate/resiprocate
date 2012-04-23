
cmake_minimum_required(VERSION 2.8)
set(APP_SRCS registrationClient.cxx AppSubsystem.cxx RegConfig.cxx)
add_executable(registrationClient ${APP_SRCS})
target_link_libraries(registrationClient rutil resip dum)

