
add_executable(queuetostream
   queuetostream.cpp
)

target_include_directories(queuetostream PUBLIC repro)
target_include_directories(queuetostream PUBLIC ${BERKELEYDB_INCLUDE_DIRS})
set_target_properties(queuetostream PROPERTIES FOLDER repro/tools)
target_link_libraries(queuetostream PUBLIC reprolib)

install(TARGETS queuetostream DESTINATION ${CMAKE_INSTALL_SBINDIR})

