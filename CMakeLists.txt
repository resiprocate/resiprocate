CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9) 

PROJECT(reSIProcate)

ENABLE_TESTING()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/cmake_modules/")

# TODO: Add support for config.h as present in current resip autoconf build -
# if needed

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
SET(WITH_SSL true CACHE BOOL "Link against SSL libraries")
SET(WITH_C_ARES false CACHE BOOL "Link against libc-ares (rather than rutil/dns/ares)")
SET(OPENSSL_ROOT_DIR "" CACHE STRING "Root installation of OpenSSL (to specify a given version)")

### Dependencies ###

# OpenSSL
IF(${WITH_SSL})
   # SSL support is requested, so make it mandatory when calling FIND_PACKAGE
   FIND_PACKAGE(OpenSSL REQUIRED)
   ADD_DEFINITIONS(-DUSE_SSL)
   IF(${OPENSSL_FOUND})
      INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
   ELSE()
      MESSAGE(FATAL_ERROR "Could not find OpenSSL")
   ENDIF()
ENDIF()

# ares
IF(${WITH_C_ARES})
   # Don't use built-in ares
   FIND_PACKAGE(cares REQUIRED)
   ADD_DEFINITIONS(-DUSE_CARES)

   SET(ARES_LIBRARIES ${CARES_LIBRARIES})
ELSE()
   # Use built-in ares
   ADD_DEFINITIONS(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   INCLUDE_DIRECTORIES(BEFORE rutil/dns/ares)
   
   SET(ARES_LIBRARIES ares)
ENDIF()

# popt
FIND_PACKAGE(popt)
IF(${POPT_FOUND})
   ADD_DEFINITIONS(-DHAVE_POPT_H)
ELSE()
   SET(POPT_LIBRARIES "")
ENDIF()

# Python
INCLUDE(FindPythonLibs)

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

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

ADD_SUBDIRECTORY(rutil)
ADD_SUBDIRECTORY(resip)
