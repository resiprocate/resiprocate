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
endif()

target_include_directories(resipares PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

if(APPLE)
   target_link_libraries(resipares PRIVATE "-framework CoreFoundation")
   target_link_libraries(resipares PRIVATE "-framework SystemConfiguration")
endif()

set_target_properties(resipares PROPERTIES VERSION ${SO_RELEASE})
set_target_properties(resipares PROPERTIES FOLDER ares)

install(TARGETS resipares DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rutil/dns/ares)

add_executable(ahost EXCLUDE_FROM_ALL ahost.c)
add_executable(adig EXCLUDE_FROM_ALL adig.c)

target_include_directories(ahost PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(adig PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(ahost resipares)
target_link_libraries(adig resipares)

set_target_properties(ahost PROPERTIES FOLDER ares)
set_target_properties(adig PROPERTIES FOLDER ares)
