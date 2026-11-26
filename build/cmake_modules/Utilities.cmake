function(target_add_conditional_sources target condition_var)
   if(NOT ${condition_var})
      set_source_files_properties(${ARGN} PROPERTIES HEADER_FILE_ONLY TRUE)
   endif(NOT ${condition_var})
   set_property(TARGET ${target} APPEND PROPERTY SOURCES ${ARGN})
endfunction()

function(install_and_preserve_hierarchy destination)
   foreach(item ${ARGN})
      get_filename_component(item_path "${item}" PATH)
      install(FILES ${item} DESTINATION "${destination}/${item_path}")
   endforeach()
endfunction()

function(test_base target)
   add_executable(${target} ${ARGN})
   add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
   # Don't strip asserts in release builds
   target_compile_options(${target} PRIVATE -UNDEBUG)
   set_tests_properties(${target} PROPERTIES TIMEOUT 300)
endfunction()

function(add_fuzzing_executable target)
   add_executable(${target} ${ARGN})
   set_target_properties(${target} PROPERTIES LINKER_LANGUAGE "CXX")
   set(_fuzz_dir ${resiprocate_SOURCE_DIR}/build/oss-fuzz)
   # for FuzzStrict.h
   target_include_directories(${target} PRIVATE ${_fuzz_dir})
   if(DEFINED ENV{LIB_FUZZING_ENGINE})
      target_link_libraries(${target} PRIVATE $ENV{LIB_FUZZING_ENGINE})
   else()
      # emulate libfuzzer entry point
      target_sources(${target} PRIVATE ${_fuzz_dir}/main.cpp)

      if(BUILD_TESTING)
         # Replay build/oss-fuzz/<target>-seeds in strict mode: every seed
         # must reach and be accepted by its parser (see FuzzStrict.h).
         # Seeds named bad-* are deliberately invalid and are replayed
         # without strict mode, together with build/oss-fuzz/<target>-regressions,
         # which holds inputs that fuzzing once found problems with.
         file(GLOB _seeds CONFIGURE_DEPENDS ${_fuzz_dir}/${target}-seeds/*)
         set(_bad ${_seeds})
         list(FILTER _seeds EXCLUDE REGEX "/bad-[^/]*$")
         list(FILTER _bad INCLUDE REGEX "/bad-[^/]*$")
         file(GLOB _regressions CONFIGURE_DEPENDS ${_fuzz_dir}/${target}-regressions/*)
         list(APPEND _bad ${_regressions})
         if(_seeds)
            add_test(NAME ${target}-seeds COMMAND $<TARGET_FILE:${target}> ${_seeds})
            set_tests_properties(${target}-seeds PROPERTIES
               ENVIRONMENT "RESIP_FUZZ_STRICT=1" TIMEOUT 120)
         endif()
         if(_bad)
            add_test(NAME ${target}-regressions COMMAND $<TARGET_FILE:${target}> ${_bad})
            set_tests_properties(${target}-regressions PROPERTIES TIMEOUT 120)
         endif()
      endif()
   endif()
endfunction()
