
add_executable(queuetostream
   queuetostream.cpp
)

target_include_directories(queuetostream PUBLIC repro)
target_include_directories(queuetostream PUBLIC ${BERKELEYDB_INCLUDE_DIRS})
target_link_libraries(queuetostream PUBLIC repro)

install(TARGETS queuetostream DESTINATION ${CMAKE_INSTALL_SBINDIR})

