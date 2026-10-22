# Try to find c-ares.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  ARES_FOUND - System has c-ares
#  ARES_INCLUDE_DIRS - The c-ares include directories
#  ARES_LIBRARIES - The libraries needed to use c-ares
#  ARES_DEFINITIONS - Compiler switches required for using c-ares

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_ARES QUIET cares)
SET(ARES_DEFINITIONS ${PC_ARES_CFLAGS_OTHER})

FIND_PATH(ARES_INCLUDE_DIR ares.h
          HINTS ${PC_ARES_INCLUDEDIR} ${PC_ARES_INCLUDE_DIRS} PATH_SUFFIXES ares)

FIND_LIBRARY(ARES_LIBRARY NAMES cares
             HINTS ${PC_ARES_LIBDIR} ${PC_ARES_LIBRARY_DIRS})

SET(ARES_LIBRARIES ${ARES_LIBRARY})
SET(ARES_INCLUDE_DIRS ${ARES_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(cares DEFAULT_MSG
                                  ARES_LIBRARY ARES_INCLUDE_DIR)

MARK_AS_ADVANCED(ARES_INCLUDE_DIR ARES_LIBRARY)
