
# FIXME: we should put the generated files in ${PROJECT_BINARY_DIR}
#        and if we don't run sox and xxd, we should copy the cached versions
#        to ${PROJECT_BINARY_DIR}
function(wav_to_header)
   if(REGENERATE_MEDIA_SAMPLES)
      add_custom_command(
         DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}.wav
         OUTPUT ${ARGV0}.h
         COMMAND sox ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}.wav -r 8k -e signed -b 16 -c 1 ${ARGV0}.raw
         COMMAND xxd -i -u ${ARGV0}.raw ${ARGV0}.h.0
         COMMAND sed -e 's/_raw//g' ${ARGV0}.h.0 > ${ARGV0}.h
         COMMAND ${CMAKE_COMMAND} -E rm ${ARGV0}.raw ${ARGV0}.h.0
      )
   else()
      file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
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

add_executable(prompt2wav
   prompt2wav.cxx
   ${INCLUDES}
)

target_include_directories(prompt2wav PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# don't need to install this, it is only used during development
#install(TARGETS prompt2wav DESTINATION ${CMAKE_INSTALL_BINDIR})

