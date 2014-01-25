CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9) 

PROJECT(reSIProcate)

ENABLE_TESTING()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/cmake_modules/")

# TODO: Add doxygen support

# TODO: Add version numbers to generated targets (e.g. libresip-1.9.so)

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
SET(WITH_SSL true CACHE BOOL "Link against SSL libraries")
SET(WITH_C_ARES false CACHE BOOL "Link against libc-ares (rather than rutil/dns/ares)")
SET(OPENSSL_ROOT_DIR "" CACHE STRING "Root installation of OpenSSL (to specify a given version)")

####################
### Dependencies ###
####################

# OpenSSL
IF(${WITH_SSL})
   # SSL support is requested, so make it mandatory when calling FIND_PACKAGE
   FIND_PACKAGE(OpenSSL REQUIRED)
   IF(${OPENSSL_FOUND})
      SET(USE_SSL true)
      ADD_DEFINITIONS(-DUSE_SSL)
      INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
   ELSE()
      MESSAGE(FATAL_ERROR "Could not find OpenSSL")
   ENDIF()
ELSE()
   # Explicitly set OPENSSL_FOUND to false since we didn't even run
   # FIND_PACKAGE on it. It needs to be set to false for other CMake scripts to
   # no it is not being used.
   SET(OPENSSL_FOUND "FALSE")
ENDIF()

# ares
IF(${WITH_C_ARES})
   # Don't use built-in ares
   FIND_PACKAGE(cares REQUIRED)

   SET(USE_CARES true)
   ADD_DEFINITIONS(-DUSE_CARES)

   SET(ARES_LIBRARIES ${CARES_LIBRARIES})
   INCLUDE_DIRECTORIES(${CARES_INCLUDE_DIRS})
ELSE()
   # Use built-in ares
   SET(USE_ARES true)
   ADD_DEFINITIONS(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   INCLUDE_DIRECTORIES(BEFORE rutil/dns/ares)
   
   SET(ARES_LIBRARIES ares)
ENDIF()

# popt
FIND_PACKAGE(popt)
IF(${POPT_FOUND})
   SET(HAVE_POPT_H true)
   ADD_DEFINITIONS(-DHAVE_POPT_H)
   INCLUDE_DIRECTORIES(${POPT_INCLUDE_DIRS})
ELSE()
   SET(POPT_LIBRARIES "")
ENDIF()

# Boost.Asio
# TODO: Is using Boost's asio OK? Or should it use the old/independant asio
# libs?
FIND_PACKAGE(Boost)
IF(${Boost_FOUND})
   INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ENDIF()

# SRTP
FIND_PACKAGE(srtp)
IF(${SRTP_FOUND})
   INCLUDE_DIRECTORIES(${SRTP_INCLUDE_DIRS})
ENDIF()

# Python
#INCLUDE(FindPythonLibs)

# Cajun
FIND_PACKAGE(cajun)
IF(${CAJUN_FOUND})
   INCLUDE_DIRECTORIES(${CAJUN_INCLUDE_DIRS})
ENDIF()

# BerkeleyDb
FIND_PACKAGE(BerkeleyDb)
IF(${BERKELEYDB_FOUND})
   INCLUDE_DIRECTORIES(${BERKELEYDB_INCLUDE_DIRS})
   SET(DB_HEADER "db_cxx.h")
ENDIF()

# epoll
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/epoll.h HAVE_EPOLL)

# monotonic clock
INCLUDE(CheckCSourceRuns)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   SET(CMAKE_REQUIRED_LIBRARIES rt)
ENDIF()

CHECK_C_SOURCE_RUNS("
   #include <time.h>
   int main() {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return 0;
   }" HAVE_CLOCK_GETTIME_MONOTONIC)

IF(${HAVE_CLOCK_GETTIME_MONOTONIC})
   ADD_DEFINITIONS(-DHAVE_CLOCK_GETTIME_MONOTONIC)
ENDIF()

##############################
### Generation of config.h ###
##############################
# TODO - Bring more values from autotools
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# This helper is used by the unit tests
FUNCTION(TEST_BASE)
   ADD_EXECUTABLE(
      ${ARGV0}
      ${ARGV1}
      ${ARGV2}
      ${ARGV3}
      ${ARGV4}
      ${ARGV5}
      ${ARGV6}
      ${ARGV7}
      ${ARGV8}
      ${ARGV9}
      ${ARGV10}
   )
   ADD_TEST(${ARGV0} ${ARGV0})
ENDFUNCTION(TEST_BASE)

# Used to group targets together when CMake generates projects for IDEs
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

ADD_SUBDIRECTORY(rutil)
ADD_SUBDIRECTORY(resip)

# CMake repro support is still in the works...
IF(${CAJUN_FOUND})
# TODO: repro
#   ADD_SUBDIRECTORY(repro)
ELSE()
   MESSAGE("-- Missing cajun, will not build repro")
ENDIF()

IF(${OPENSSL_FOUND} AND ${Boost_FOUND} AND ${SRTP_FOUND})
   ADD_SUBDIRECTORY(reTurn)
   ADD_SUBDIRECTORY(reflow)
ELSE()
   MESSAGE("-- Missing libraries, will not build reTurn")
   MESSAGE("-- Missing libraries, will not build reFlow")
ENDIF()
