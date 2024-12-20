# Try to find Netxx.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  NETXX_FOUND - System has netxx
#  NETXX_INCLUDE_DIRS - The netxx include directories
#  NETXX_LIBRARIES - The libraries needed to use netxx
#  NETXX_DEFINITIONS - Compiler switches required for using netxx

# If pkg-config is present, use its results as hints for FIND_*, otherwise
# don't
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_NETXX QUIET libnetxx-dev)  # NetXX doesn't include a .pc file, so this will fail
SET(NETXX_DEFINITIONS ${PC_NETXX_CFLAGS_OTHER})

FIND_PATH(NETXX_INCLUDE_DIR Netbuf.h
          HINTS ${PC_NETXX_INCLUDEDIR} ${PC_NETXX_INCLUDE_DIRS} /usr/include/Netxx)

FIND_LIBRARY(NETXX_LIBRARY NAMES Netxx
             HINTS ${PC_NETXX_LIBDIR} ${PC_NETXX_LIBRARY_DIRS})

SET(NETXX_LIBRARIES ${NETXX_LIBRARY})
SET(NETXX_INCLUDE_DIRS ${NETXX_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Netxx  DEFAULT_MSG
                                  NETXX_LIBRARY NETXX_INCLUDE_DIR)

MARK_AS_ADVANCED(NETXX_INCLUDE_DIR NETXX_LIBRARY)
