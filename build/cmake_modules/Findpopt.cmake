# Try to find popt.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  POPT_FOUND - System has popt
#  POPT_INCLUDE_DIRS - The popt include directories
#  POPT_LIBRARIES - The libraries needed to use popt
#  POPT_DEFINITIONS - Compiler switches required for using popt

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_POPT QUIET libpopt)
SET(POPT_DEFINITIONS ${PC_POPT_CFLAGS_OTHER})

FIND_PATH(POPT_INCLUDE_DIR popt.h
          HINTS ${PC_POPT_INCLUDEDIR} ${PC_POPT_INCLUDE_DIRS})

FIND_LIBRARY(POPT_LIBRARY NAMES popt
             HINTS ${PC_POPT_LIBDIR} ${PC_POPT_LIBRARY_DIRS})

SET(POPT_LIBRARIES ${POPT_LIBRARY})
SET(POPT_INCLUDE_DIRS ${POPT_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(popt  DEFAULT_MSG
                                  POPT_LIBRARY POPT_INCLUDE_DIR)

MARK_AS_ADVANCED(POPT_INCLUDE_DIR POPT_LIBRARY)
