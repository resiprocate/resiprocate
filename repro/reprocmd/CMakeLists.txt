
add_executable(reprocmd
   reprocmd.cpp
)

target_link_libraries(reprocmd PUBLIC repro)

install(TARGETS reprocmd DESTINATION ${CMAKE_INSTALL_SBINDIR})
