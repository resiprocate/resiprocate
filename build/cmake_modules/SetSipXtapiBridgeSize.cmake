# Set DEFAULT_BRIDGE_MAX_IN_OUTPUTS in sipXtapi's Visual Studio projects.
#
# Usage: cmake -DSIPXTAPI_SOURCE_DIR=<dir> -DVALUE=<n> -P SetSipXtapiBridgeSize.cmake
#
# sipXmediaLib's and sipXmediaAdapterLib's .vcxproj files hard-code
# DEFAULT_BRIDGE_MAX_IN_OUTPUTS=10 in their PreprocessorDefinitions, and recon
# must be compiled with the same value as sipXtapi.  The top-level
# CMakeLists.txt runs this as the configure step of the Windows sipXtapi
# ExternalProject, so that the value comes from reSIProcate's
# DEFAULT_BRIDGE_MAX_IN_OUTPUTS cache variable.
#
# The existing value is replaced rather than a second definition appended,
# which would only draw a macro redefinition warning and leave the result
# depending on definition order.  Files are rewritten only if they change, so
# an unchanged value does not make msbuild rebuild anything.

if(NOT SIPXTAPI_SOURCE_DIR OR NOT VALUE)
   message(FATAL_ERROR "SetSipXtapiBridgeSize.cmake needs SIPXTAPI_SOURCE_DIR and VALUE")
endif()

file(GLOB _vcxproj_files
     "${SIPXTAPI_SOURCE_DIR}/sipXmediaLib/*.vcxproj"
     "${SIPXTAPI_SOURCE_DIR}/sipXmediaAdapterLib/*.vcxproj")

set(_found FALSE)
foreach(_vcxproj IN LISTS _vcxproj_files)
   file(READ "${_vcxproj}" _content)
   if(NOT _content MATCHES "DEFAULT_BRIDGE_MAX_IN_OUTPUTS=[0-9]+")
      continue()
   endif()
   set(_found TRUE)
   string(REGEX REPLACE "DEFAULT_BRIDGE_MAX_IN_OUTPUTS=[0-9]+"
          "DEFAULT_BRIDGE_MAX_IN_OUTPUTS=${VALUE}" _new_content "${_content}")
   if(NOT _new_content STREQUAL _content)
      file(WRITE "${_vcxproj}" "${_new_content}")
      message(STATUS "Set DEFAULT_BRIDGE_MAX_IN_OUTPUTS=${VALUE} in ${_vcxproj}")
   endif()
endforeach()

# If upstream stops defining it in the projects, fail loudly: silently leaving
# sipXtapi at its header default could mismatch recon.
if(NOT _found)
   message(FATAL_ERROR "No sipXtapi Visual Studio project under ${SIPXTAPI_SOURCE_DIR} defines DEFAULT_BRIDGE_MAX_IN_OUTPUTS; cannot set it to ${VALUE}")
endif()
