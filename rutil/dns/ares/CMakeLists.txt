SET(INCLUDES
   ares.h
   ares_dns.h
   ares_private.h
   ares_version.h
   ares_compat.h
   ares_local.h
   ares_socketfunc.h
)

SET(SOURCES
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
   ares_process.c
   ares_query.c
   ares_search.c
   ares_send.c
   ares_strerror.c
   ares_timeout.c
)

INCLUDE_DIRECTORIES(.)

IF(CMAKE_HOST_WIN32)
   # Use static .lib on Windows because resip ares doesn't export any symbols
   ADD_LIBRARY(ares ${SOURCES})
   TARGET_LINK_LIBRARIES(ares ws2_32)
ELSE()
   ADD_LIBRARY(ares SHARED ${SOURCES})
ENDIF()

ADD_EXECUTABLE(ahost ahost.c)
ADD_EXECUTABLE(adig adig.c)

TARGET_LINK_LIBRARIES(ahost ares)
TARGET_LINK_LIBRARIES(adig ares)

SET_TARGET_PROPERTIES(ares PROPERTIES FOLDER ares)
SET_TARGET_PROPERTIES(ahost PROPERTIES FOLDER ares)
SET_TARGET_PROPERTIES(adig PROPERTIES FOLDER ares)

# TODO: Do we need to also install ahost + adig?
INSTALL(TARGETS ares DESTINATION lib)
# TODO: Where to put resip ares' include files? Straight under
# /usr/local/include or under rutil/...?
INSTALL(FILES ${INCLUDES} DESTINATION include)
