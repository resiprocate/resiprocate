
cmake_minimum_required(VERSION 2.8)
set(APP_SRCS registrationClient.cxx AppSubsystem.cxx KeyedFile.cxx RegConfig.cxx UserAccount.cxx UserRegistrationClient.cxx)
add_executable(registrationClient ${APP_SRCS})
target_link_libraries(registrationClient rutil resip dum)

