function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER tfm/repro)
   target_link_libraries(${ARGV0} PUBLIC tfmrepro)
   set_tests_properties(${ARGV0} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
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

target_link_libraries(tfmrepro PUBLIC repro tfm ${POPT_LIBRARIES})
version_libname(tfmrepro)

test(sanityTests sanityTests.cxx)

install(TARGETS tfmrepro DESTINATION ${CMAKE_INSTALL_LIBDIR})

