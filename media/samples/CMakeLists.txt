
# FIXME: we should put the generated files in ${PROJECT_BINARY_DIR}
#        and if we don't run sox and xxd, we should copy the cached versions
#        to ${PROJECT_BINARY_DIR}
function(wav_to_header)
   if(REGENERATE_MEDIA_SAMPLES)
      find_program(SOX_EXECUTABLE "sox") # with CMake 3.18, use REQUIRED
      if(NOT SOX_EXECUTABLE)
         message(FATAL_ERROR "The sox utility cannot be found.")
      endif()
      add_custom_command(
         DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}.wav
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.h
         COMMAND ${SOX_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}.wav -r 8k -e signed -b 16 -c 1 ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.raw
         COMMAND xxd -i -u ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.raw ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.h.0
         COMMAND sed -e "s/^unsigned char .*_raw/unsigned char ${ARGV0}/g" -e "s/^unsigned int .*_raw_len/unsigned int ${ARGV0}_len/g" ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.h.0 > ${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.h
      )
   else()
      file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cache/${ARGV0}.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
   endif()
endfunction()

wav_to_header(playback_prompt)
wav_to_header(record_prompt)
wav_to_header(music)

set(INCLUDES
   playback_prompt.h
   record_prompt.h
   music.h
)

set_source_files_properties(
   playback_prompt.h
   record_prompt.h
   music.h
   PROPERTIES GENERATED TRUE
)

add_executable(prompt2wav
   prompt2wav.cxx
   ${INCLUDES}
)

#target_include_directories(prompt2wav PRIVATE ${CMAKE_BINARY_DIR})

# don't need to install this, it is only used during development
#install(TARGETS prompt2wav DESTINATION ${CMAKE_INSTALL_BINDIR})

