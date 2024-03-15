/*
 More information can be obtained from
 http://www.cmake.org/Wiki/CMake:How_To_Write_Platform_Checks
*/

#cmakedefine GPERF_SIZE_TYPE @GPERF_SIZE_TYPE@
#cmakedefine HAVE_EPOLL 1
#cmakedefine USE_MYSQL
#cmakedefine USE_POSTGRESQL
#cmakedefine USE_MAXMIND_GEOIP
#cmakedefine01 HAVE_CLOCK_GETTIME_MONOTONIC
#define DEFAULT_BRIDGE_MAX_IN_OUTPUTS @DEFAULT_BRIDGE_MAX_IN_OUTPUTS@
#cmakedefine SIPX_NO_RECORD

#cmakedefine ENABLE_LOG_REPOSITORY_DETAILS
#cmakedefine RESIPROCATE_GIT_ID "@RESIPROCATE_GIT_ID@"
#cmakedefine RESIPROCATE_BRANCH_NAME "@RESIPROCATE_BRANCH_NAME@"
#define VERSION "@PROJECT_VERSION@"
#cmakedefine RESIP_BIG_ENDIAN
#cmakedefine REPRO_BUILD_REV "@REPRO_BUILD_REV@"
#cmakedefine REPRO_BUILD_HOST "localhost"
#cmakedefine DB_HEADER "@DB_HEADER@"
#cmakedefine RESIP_SIP_MSG_MAX_BYTES @RESIP_SIP_MSG_MAX_BYTES@
#cmakedefine USE_SIGCOMP


// TIME_WITH_SYS_TIME
// STDC_HEADERS
// more ...
