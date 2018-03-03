# Try to find c-ares.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  CARES_FOUND - System has c-ares
#  CARES_INCLUDE_DIRS - The c-ares include directories
#  CARES_LIBRARIES - The libraries needed to use c-ares
#  CARES_DEFINITIONS - Compiler switches required for using c-ares

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_CARES QUIET libcares)
SET(CARES_DEFINITIONS ${PC_CARES_CFLAGS_OTHER})

FIND_PATH(CARES_INCLUDE_DIR ares.h
          HINTS ${PC_CARES_INCLUDEDIR} ${PC_CARES_INCLUDE_DIRS})

FIND_LIBRARY(CARES_LIBRARY NAMES cares
             HINTS ${PC_CARES_LIBDIR} ${PC_CARES_LIBRARY_DIRS})

SET(CARES_LIBRARIES ${CARES_LIBRARY})
SET(CARES_INCLUDE_DIRS ${CARES_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(cares  DEFAULT_MSG
                                  CARES_LIBRARY CARES_INCLUDE_DIR)

MARK_AS_ADVANCED(CARES_INCLUDE_DIR CARES_LIBRARY)
