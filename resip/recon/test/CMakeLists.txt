function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER recon/Tests)
   target_link_libraries(${ARGV0} recon)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
   if(WIN32)
     if(USE_SIPXTAPI)
       if(SIPXTAPI_PROJS_IN_VS_GUI)
         add_dependencies(${ARGV0} ${SIPXTAPI_VSPROJ_DEPS})
       endif()
       add_custom_command ( TARGET ${ARGV0} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
         $<TARGET_FILE:SIPXTAPI_CODEC_PLUGINS> $<TARGET_FILE_DIR:${ARGV0}>
         COMMAND_EXPAND_LISTS
       )
     endif()
     if(WITH_SSL)
       add_custom_command ( TARGET ${ARGV0} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
         $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
         COMMAND_EXPAND_LISTS
       )
     endif()
   endif()
endfunction()

# These are failing on some linux builds - TODO - fix me
if(WIN32)
  test(unitTests sdpTests.cxx unitTests.cxx)
endif()

add_executable(testUA
   testUA.cxx
)

target_link_libraries(testUA recon)
if(WIN32)
  if(USE_SIPXTAPI)
    if(SIPXTAPI_PROJS_IN_VS_GUI)
      add_dependencies(testUA ${SIPXTAPI_VSPROJ_DEPS})
    endif()
    add_custom_command ( TARGET testUA POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SIPXTAPI_CODEC_PLUGINS> $<TARGET_FILE_DIR:testUA>
      COMMAND_EXPAND_LISTS
    )
  endif()
  if(WITH_SSL)
    add_custom_command ( TARGET testUA POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:testUA>
      COMMAND_EXPAND_LISTS
    )
  endif()
endif()

install(TARGETS testUA DESTINATION ${CMAKE_INSTALL_BINDIR})

set_target_properties(testUA PROPERTIES FOLDER recon/Apps)


