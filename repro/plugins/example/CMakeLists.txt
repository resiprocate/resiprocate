
add_library(example
   ExamplePlugin.cxx
)

install(TARGETS example DESTINATION ${INSTALL_REPRO_PLUGIN_DIR})
