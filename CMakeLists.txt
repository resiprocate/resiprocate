CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9) 

PROJECT(reSIProcate)

ENABLE_TESTING()

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
SET(WITH_SSL true CACHE BOOL "Link against SSL libraries")
SET(WITH_C_ARES false CACHE BOOL "Link against libc-ares (rather than rutil/dns/ares)")
SET(OPENSSL_ROOT_DIR "" CACHE STRING "Root installation of OpenSSL (to specify a given version)")

### OpenSSL
IF(${WITH_SSL})
   # Use the built-in CMake macro to find OpenSSL
   INCLUDE(FindOpenSSL)
   ADD_DEFINITIONS(-DUSE_SSL)
   IF(${OPENSSL_FOUND})
      INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
   ELSE()
      MESSAGE(FATAL_ERROR "Could not find OpenSSL")
   ENDIF()
ENDIF()

### ares
IF(NOT ${WITH_C_ARES})
   ADD_DEFINITIONS(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   INCLUDE_DIRECTORIES(BEFORE rutil/dns/ares)
ELSE()
   # TODO: Use FIND_LIBRARY to find c-ares if not installed under obvious
   # places. Otherwise it will try to find it in the system's default paths
   # (e.g. /usr/lib, ...).
   ADD_DEFINITIONS(-DUSE_CARES)
ENDIF()

### Python
INCLUDE(FindPythonLibs)

### popt
#FIND_PACKAGE(popt)

# This helper is used by the unit tests
FUNCTION(TEST_BASE)
   ADD_EXECUTABLE(${ARGV0} ${ARGV1} ${ARGV2} ${ARGV3} ${ARGV4})
   ADD_TEST(${ARGV0} ${ARGV0})
ENDFUNCTION(TEST_BASE)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

ADD_SUBDIRECTORY(rutil)
ADD_SUBDIRECTORY(resip)
