# Try to find srtp.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  SRTP2_FOUND - System has srtp
#  SRTP2_INCLUDE_DIRS - The srtp include directories
#  SRTP2_LIBRARIES - The libraries needed to use srtp
#  SRTP2_DEFINITIONS - Compiler switches required for using srtp

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_SRTP QUIET libsrtp2)
SET(SRTP2_DEFINITIONS ${PC_SRTP2_CFLAGS_OTHER})

FIND_PATH(SRTP2_INCLUDE_DIR srtp.h
        HINTS ${PC_SRTP2_INCLUDEDIR} ${PC_SRTP2_INCLUDE_DIRS} PATH_SUFFIXES srtp2)

FIND_LIBRARY(SRTP2_LIBRARY NAMES srtp2
        HINTS ${PC_SRTP2_LIBDIR} ${PC_SRTP2_LIBRARY_DIRS})

SET(SRTP2_LIBRARIES ${SRTP2_LIBRARY})
SET(SRTP2_INCLUDE_DIRS ${SRTP2_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(srtp2 DEFAULT_MSG
        SRTP2_LIBRARY SRTP2_INCLUDE_DIR)

MARK_AS_ADVANCED(SRTP2_INCLUDE_DIR SRTP2_LIBRARY)
