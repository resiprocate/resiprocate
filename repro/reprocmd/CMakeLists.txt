
add_executable(reprocmd
   reprocmd.cpp
)

set_target_properties(reprocmd PROPERTIES FOLDER repro/tools)
target_link_libraries(reprocmd PUBLIC reprolib)

install(TARGETS reprocmd DESTINATION ${CMAKE_INSTALL_SBINDIR})
