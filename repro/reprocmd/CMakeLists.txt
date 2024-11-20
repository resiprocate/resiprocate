
add_executable(reprocmd
   reprocmd.cpp
)

target_link_libraries(reprocmd PUBLIC reprolib)

install(TARGETS reprocmd DESTINATION ${CMAKE_INSTALL_SBINDIR})
