cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build/cmake_modules")

if(POLICY CMP0042)
  CMAKE_POLICY(SET CMP0042 NEW)
endif(POLICY CMP0042)

project(reSIProcate VERSION 1.12.0)

include(GNUInstallDirs)
include(Utilities)

enable_testing()

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
option(WITH_SSL "Link against SSL libraries" TRUE)
option(WITH_C_ARES "Link against libc-ares (rather than rutil/dns/ares)" FALSE)
option(BUILD_SHARED_LIBS "Build libraries as shared" TRUE)

#set(CMAKE_CXX_STANDARD 98)

####################
### Dependencies ###
####################

# Threads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

# OpenSSL
if(WITH_SSL)
   # SSL support is requested, so make it mandatory when calling find_package
   find_package(OpenSSL REQUIRED)
   if(OPENSSL_FOUND)
      set(USE_SSL true)
      add_definitions(-DUSE_SSL)
      include_directories(${OPENSSL_INCLUDE_DIR})
   endif()
else()
   # Explicitly set OPENSSL_FOUND to false since we didn't even run
   # find_package on it. It needs to be set to false for other CMake scripts to
   # no it is not being used.
   set(OPENSSL_FOUND FALSE)
endif()

# ares
if(WITH_C_ARES)
   # Don't use built-in ares
   find_package(c-ares REQUIRED)

   set(USE_CARES true)
   add_definitions(-DUSE_CARES)

   set(ARES_LIBRARIES c-ares::cares)
   include_directories(${c-ares_INCLUDE_DIR})
else()
   # Use built-in ares
   set(USE_ARES true)
   add_definitions(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   include_directories(BEFORE rutil/dns/ares)
   
   set(ARES_LIBRARIES resipares)
endif()

# popt
find_package(popt)
if(POPT_FOUND)
   set(HAVE_POPT_H true)
   add_definitions(-DHAVE_POPT_H)
   include_directories(${POPT_INCLUDE_DIRS})
endif()

# ASIO
# modern ASIO requires C++11
set(ASIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/asio")
find_package(ASIO)
if(ASIO_FOUND)
   include_directories(SYSTEM ${ASIO_INCLUDE_DIRS})
endif()

# Boost
find_package(Boost)
if(Boost_FOUND)
   include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif()

# SRTP2
find_package(srtp2)
if(SRTP2_FOUND)
   include_directories(SYSTEM ${SRTP2_INCLUDE_DIRS})
endif()

# Cajun
set(CAJUN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cajun/include")
find_package(cajun)
if(CAJUN_FOUND)
   include_directories(SYSTEM ${CAJUN_INCLUDE_DIRS})
endif()

# BerkeleyDb
find_package(BerkeleyDb)
if(BERKELEYDB_FOUND)
   include_directories(SYSTEM ${BERKELEYDB_INCLUDE_DIRS})
   set(DB_HEADER "db_cxx.h")
endif()

# GeoIP
find_package(GeoIP)
if(GeoIP_FOUND)
   set(USE_MAXMIND_GEOIP true)
endif()

# MySQL
find_package(MySQL)
if(MySQL_FOUND)
   set(USE_MYSQL true)
endif()

# PostgreSQL
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
   set(USE_POSTGRESQL true)
endif()

# gperf
set(GPERF_SIZE_TYPE "size_t")

# epoll
include(CheckIncludeFiles)
check_include_files(sys/epoll.h HAVE_EPOLL)

# monotonic clock
include(CheckCSourceRuns)

check_c_source_runs("
   #include <time.h>
   int main() {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return 0;
   }" HAVE_CLOCK_GETTIME_MONOTONIC)

if(HAVE_CLOCK_GETTIME_MONOTONIC)
   add_definitions(-DHAVE_CLOCK_GETTIME_MONOTONIC)
endif()

if(WIN32)
   add_definitions(-DNOMINMAX)
   add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# shared library versioning

set(SO_RELEASE "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

##############################
### Generation of config.h ###
##############################
# TODO - Bring more values from autotools
add_definitions(-DHAVE_CONFIG_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Used to group targets together when CMake generates projects for IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(rutil)
add_subdirectory(resip)

if(OPENSSL_FOUND AND ASIO_FOUND AND Boost_FOUND)
   add_subdirectory(reTurn)
else()
   message(STATUS "Missing libraries, will not build reTurn")
endif()

if(OPENSSL_FOUND AND ASIO_FOUND AND Boost_FOUND AND SRTP2_FOUND)
   add_subdirectory(reflow)
else()
   message(STATUS "Missing libraries, will not build reFlow")
endif()

if(BERKELEYDB_FOUND AND CAJUN_FOUND)
   add_subdirectory(repro)
else()
   message(STATUS "Missing libraries, will not build repro")
endif()

###############
### Summary ###
###############

include(FeatureSummary)
feature_summary(WHAT ALL)
