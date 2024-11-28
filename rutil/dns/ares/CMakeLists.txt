set(INCLUDES
   ares.h
   ares_compat.h
   ares_dns.h
   ares_local.h
   ares_socketfunc.h
   ares_version.h
)

set(SOURCES
   ares__close_sockets.c
   ares__get_hostent.c
   ares__read_line.c
   ares_destroy.c
   ares_expand_name.c
   ares_fds.c
   ares_free_errmem.c
   ares_free_hostent.c
   ares_free_string.c
   ares_gethostbyaddr.c
   ares_gethostbyname.c
   ares_init.c
   ares_local.c
   ares_mkquery.c
   ares_parse_a_reply.c
   ares_parse_ptr_reply.c
   ares_private.h
   ares_process.c
   ares_query.c
   ares_search.c
   ares_send.c
   ares_strerror.c
   ares_timeout.c
)

if(WIN32)
   # Use static .lib on Windows because resip ares doesn't export any symbols
   add_library(resipares STATIC ${SOURCES})
   target_link_libraries(resipares PRIVATE ws2_32)
else()
   add_library(resipares ${SOURCES} ${INCLUDES})

   check_include_files(arpa/nameser.h HAVE_ARPA_NAMESER_H)
   target_compile_definitions(resipares PUBLIC $<$<BOOL:HAVE_ARPA_NAMESER_H>:HAVE_ARPA_NAMESER_H>)
   check_include_files(arpa/nameser_compat.h HAVE_ARPA_NAMESER_COMPAT_H)
   target_compile_definitions(resipares PUBLIC $<$<BOOL:HAVE_ARPA_NAMESER_COMPAT_H>:HAVE_ARPA_NAMESER_COMPAT_H>)
endif()

target_include_directories(resipares PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

if(APPLE)
   target_link_libraries(resipares PRIVATE "-framework CoreFoundation")
   target_link_libraries(resipares PRIVATE "-framework SystemConfiguration")
endif()

version_libname(resipares)
set_target_properties(resipares PROPERTIES FOLDER rutil/ares)

install(TARGETS resipares DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rutil/dns/ares)

add_executable(ahost EXCLUDE_FROM_ALL ahost.c)
add_executable(adig EXCLUDE_FROM_ALL adig.c)

target_include_directories(ahost PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(adig PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(ahost resipares)
target_link_libraries(adig resipares)

set_target_properties(ahost PROPERTIES FOLDER rutil/ares/tools)
set_target_properties(adig PROPERTIES FOLDER rutil/ares/tools)

# fuzzing targets

add_fuzzing_executable(aresfuzz ares-test-fuzz.c)
set_target_properties(aresfuzz PROPERTIES FOLDER rutil/ares/tools)
target_link_libraries(aresfuzz PRIVATE resipares)

add_fuzzing_executable(aresfuzzname ares-test-fuzz-name.c)
set_target_properties(aresfuzzname PROPERTIES FOLDER rutil/ares/tools)
target_link_libraries(aresfuzzname PRIVATE resipares)
