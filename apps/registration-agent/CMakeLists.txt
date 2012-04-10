
cmake_minimum_required(VERSION 2.8)
set(APP_SRCS basicRegister.cxx)
add_executable(basicRegister ${APP_SRCS})
target_link_libraries(basicRegister rutil resip dum)

