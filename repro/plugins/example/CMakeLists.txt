
add_library(example MODULE
   ExamplePlugin.cxx
)

target_link_libraries(example PUBLIC reprolib ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})

install(TARGETS example DESTINATION ${INSTALL_REPRO_PLUGIN_DIR})
