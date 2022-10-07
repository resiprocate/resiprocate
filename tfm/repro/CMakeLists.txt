function(test)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER tfm/repro)
   target_link_libraries(${ARGV0} rutil tfm)
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

test(sanityTests sanityTests.cxx)

