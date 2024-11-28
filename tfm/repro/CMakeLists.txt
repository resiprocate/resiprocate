function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER tfm/tfmrepro)
   target_link_libraries(${ARGV0} PUBLIC tfmrepro)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
   if(WIN32 AND WITH_SSL)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:OPENSSL> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
   endif()
   if(WIN32 AND USE_BDB)
     add_custom_command ( TARGET ${ARGV0} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       $<TARGET_FILE:BERKELEYDB> $<TARGET_FILE_DIR:${ARGV0}>
       COMMAND_EXPAND_LISTS
     )
  endif()
endfunction()

set(INCLUDES
   CommandLineParser.hxx
   ReproFixture.hxx
   TestRepro.hxx
   TestReproUser.hxx
)

add_library(tfmrepro
   CommandLineParser.cxx
   ReproFixture.cxx
   TestRepro.cxx
   TestReproUser.cxx
   ${INCLUDES}
)

set_target_properties(tfmrepro PROPERTIES FOLDER tfm/tfmrepro)
target_include_directories(tfmrepro PRIVATE ${POPT_INCLUDE_DIRS})
target_link_libraries(tfmrepro PUBLIC reprolib tfm ${POPT_LIBRARIES})
version_libname(tfmrepro)

set(CERTS
   ${CMAKE_SOURCE_DIR}/tfm/repro/root_cert_resipCA.pem
   ${CMAKE_SOURCE_DIR}/tfm/repro/domain_key_localhost.pem 
   ${CMAKE_SOURCE_DIR}/tfm/repro/domain_cert_localhost.pem
)

if(BUILD_TESTING)
    test(sanityTests sanityTests.cxx)
    set_tests_properties(sanityTests PROPERTIES TIMEOUT 3600)  # Set timeout to 60 mins
    add_custom_command ( TARGET sanityTests POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       ${CERTS} $<TARGET_FILE_DIR:sanityTests>
       COMMAND_EXPAND_LISTS
    )
endif()

install(TARGETS tfmrepro DESTINATION ${CMAKE_INSTALL_LIBDIR})



