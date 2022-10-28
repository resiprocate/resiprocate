function(TEST)
   test_base(${ARGV})
   set_target_properties(${ARGV0} PROPERTIES FOLDER reTurn/Tests)
   target_link_libraries(${ARGV0} reTurnClient)
endfunction()

test(stunTestVectors stunTestVectors.cxx)
