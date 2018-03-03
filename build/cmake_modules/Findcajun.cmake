# Try to find cajun.
#
# Adapted from the CMake wiki page
#
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Once done this will define
#
#  CAJUN_FOUND - System has cajun
#  CAJUN_INCLUDE_DIRS - The cajun include directories

FIND_PATH(CAJUN_INCLUDE_DIR cajun/json/reader.h
          PATHS PATH_SUFFIX cajun/json)

SET(CAJUN_INCLUDE_DIRS ${CAJUN_INCLUDE_DIR})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(cajun DEFAULT_MSG
                                   CAJUN_INCLUDE_DIR)

MARK_AS_ADVANCED(CAJUN_INCLUDE_DIR)
