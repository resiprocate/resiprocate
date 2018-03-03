# Try to find srtp.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  SRTP_FOUND - System has srtp
#  SRTP_INCLUDE_DIRS - The srtp include directories
#  SRTP_LIBRARIES - The libraries needed to use srtp
#  SRTP_DEFINITIONS - Compiler switches required for using srtp

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_SRTP QUIET libsrtp)
SET(SRTP_DEFINITIONS ${PC_SRTP_CFLAGS_OTHER})

FIND_PATH(SRTP_INCLUDE_DIR srtp.h
          HINTS ${PC_SRTP_INCLUDEDIR} ${PC_SRTP_INCLUDE_DIRS} PATH_SUFFIXES srtp)

FIND_LIBRARY(SRTP_LIBRARY NAMES srtp
             HINTS ${PC_SRTP_LIBDIR} ${PC_SRTP_LIBRARY_DIRS})

SET(SRTP_LIBRARIES ${SRTP_LIBRARY})
SET(SRTP_INCLUDE_DIRS ${SRTP_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(srtp  DEFAULT_MSG
                                  SRTP_LIBRARY SRTP_INCLUDE_DIR)

MARK_AS_ADVANCED(SRTP_INCLUDE_DIR SRTP_LIBRARY)
