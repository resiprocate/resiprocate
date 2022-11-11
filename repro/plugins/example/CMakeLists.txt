
add_library(example MODULE
   ExamplePlugin.cxx
)

install(TARGETS example DESTINATION ${INSTALL_REPRO_PLUGIN_DIR})
