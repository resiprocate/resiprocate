# Try to find ASIO.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  ASIO_FOUND - System has ASIO
#  ASIO_INCLUDE_DIRS - The ASIO include directories
#  ASIO_LIBRARIES - The libraries needed to use ASIO
#  ASIO_DEFINITIONS - Compiler switches required for using ASIO

FIND_PATH(ASIO_INCLUDE_DIR asio.hpp)

SET(ASIO_INCLUDE_DIRS ${ASIO_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(ASIO DEFAULT_MSG
                                  ASIO_INCLUDE_DIR)

MARK_AS_ADVANCED(ASIO_INCLUDE_DIR)
