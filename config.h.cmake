/*
 More information can be obtained from
 http://www.cmake.org/Wiki/CMake:How_To_Write_Platform_Checks
*/

#cmakedefine DB_HEADER "@DB_HEADER@"
#cmakedefine GPERF_SIZE_TYPE @GPERF_SIZE_TYPE@
#cmakedefine HAVE_EPOLL 1
#cmakedefine HAVE_POPT_H 1
#cmakedefine USE_SSL 1
#cmakedefine USE_CARES 1
#cmakedefine USE_ARES 1
#cmakedefine USE_MYSQL 1
#cmakedefine USE_POSTGRESQL 1
#cmakedefine USE_MAXMIND_GEOIP
#cmakedefine01 HAVE_CLOCK_GETTIME_MONOTONIC

// USE_IPV6
// USE_DTLS
// TIME_WITH_SYS_TIME
// STDC_HEADERS
// RESIP_SIP_MSG_MAX_BYTES
// RESIP_HAVE_RADCLI
// RESIP_HAVE_FREERADIUS_CLIENT
// more ...
