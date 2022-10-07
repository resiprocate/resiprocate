cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules")

if(POLICY CMP0042)
  CMAKE_POLICY(SET CMP0042 NEW)
endif(POLICY CMP0042)
if(POLICY CMP0077)
   CMAKE_POLICY(SET CMP0077 NEW)
endif(POLICY CMP0077)

project(resiprocate VERSION 1.13.0)

include(GNUInstallDirs)
include(Utilities)

enable_testing()

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
option(WITH_SSL "Link against SSL libraries" TRUE)
option(WITH_C_ARES "Link against libc-ares (rather than rutil/dns/ares)" FALSE)
if(WIN32)
   set(BUILD_SHARED_LIBS_DEFAULT OFF)
else()
   set(BUILD_SHARED_LIBS_DEFAULT ON)
endif()
option(BUILD_SHARED_LIBS "Build libraries as shared" ${BUILD_SHARED_LIBS_DEFAULT})

set(CMAKE_CXX_STANDARD 11)

if(USE_SIPXTAPI OR USE_KURENTO)
   set(DEFAULT_BRIDGE_MAX_IN_OUTPUTS 20)
endif()

####################
### Dependencies ###
####################

find_package(PkgConfig)

# Threads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

# ares
if(WITH_C_ARES)
   # Don't use built-in ares
   find_package(cares REQUIRED)

   set(USE_CARES true)
   add_definitions(-DUSE_CARES)

   set(ARES_LIBRARIES cares)
   include_directories(${cares_INCLUDE_DIR})
else()
   # Use built-in ares
   set(USE_ARES true)
   add_definitions(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   include_directories(BEFORE rutil/dns/ares)
   
   set(ARES_LIBRARIES resipares)
endif()

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

# SRTP2
# Debian: libsrtp2-dev
find_package(srtp2)
if(SRTP2_FOUND)
   include_directories(SYSTEM ${SRTP2_INCLUDE_DIRS})
endif()

# popt
# Debian: libpopt-dev
if(WIN32)
   add_subdirectory(contrib/popt)
   set(POPT_LIBRARIES popt)
   set(HAVE_POPT_H true)
   add_definitions(-DHAVE_POPT_H)
else()
   find_package(popt)
   if(POPT_FOUND)
      set(HAVE_POPT_H true)
      add_definitions(-DHAVE_POPT_H)
      include_directories(${POPT_INCLUDE_DIRS})
   endif()
endif()

# sigcomp
# FIXME
#find_package(OpenSigComp)

# fmt
# Debian: libfmt-dev
find_package(fmt)

# MySQL
# Debian: default-libmysqlclient-dev
find_package(MySQL)
if(MySQL_FOUND)
   set(USE_MYSQL true)
endif()

# soci (MySQL, PostgreSQL)
# Debian: libsoci-dev
# FIXME - hardcoded
# FIXME - MySQL, PostgreSQL)
set(SOCI_FOUND true)
set(SOCI_INCLUDE_DIRS "/usr/include/soci")
set(SOCI_LIBRARIES soci_core)

# PostgreSQL
# Debian: libpq-dev postgresql-server-dev-all
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
   set(USE_POSTGRESQL true)
endif()

# GeoIP
# Debian: libgeoip-dev
find_package(GeoIP)
if(GeoIP_FOUND)
   set(USE_MAXMIND_GEOIP true)
endif()

# radcli (RADIUS client)
# Debian: libradcli-dev
pkg_check_modules(LIBRADIUS radcli)
if(LIBRADIUS_FOUND)
   set(USE_RADIUS_CLIENT true)
endif()

# NetSNMP
# Debian: libsnmp-dev
pkg_check_modules(NETSNMP_AGENT netsnmp-agent)

# BerkeleyDb
# Debian: libdb++-dev
find_package(BerkeleyDb)
if(BERKELEYDB_FOUND)
   include_directories(SYSTEM ${BERKELEYDB_INCLUDE_DIRS})
   set(DB_HEADER "db_cxx.h")
endif()

# PCRE
# Debian: libpcre3-dev
pkg_check_modules(PCRE libpcre)

# Cajun
# Debian: libcajun-dev
#set(CAJUN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cajun/include")
find_package(cajun)
if(CAJUN_FOUND)
   include_directories(SYSTEM ${CAJUN_INCLUDE_DIRS})
endif()

# ASIO
# Debian: libasio-dev
# modern ASIO requires C++11
#set(ASIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/asio")
find_package(ASIO)
if(ASIO_FOUND)
   include_directories(SYSTEM ${ASIO_INCLUDE_DIRS})
endif()

# Boost
# Debian: libboost-all-dev
find_package(Boost)
if(Boost_FOUND)
   include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif()

if(BUILD_TELEPATHY_CM)
   # Telepathy-Qt
   # Debian: libtelepathy-qt5-dev
   find_package(TelepathyQt5)
endif()

# sipXtapi
# Debian: libsipxtapi-dev
# FIXME - hardcoded
set(SIPXTAPI_FOUND true)
set(SIPXTAPI_INCLUDE_DIRS "/usr/include/sipxtapi")
set(SIPXTAPI_LIBRARIES sipXport sipXsdp sipXmedia sipXmediaProcessing)
set(SIPX_NO_RECORD true)
# FIXME - WIN32:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__pingtel_on_posix__")

# Python
# Debian: python3-dev
find_package(Python3 COMPONENTS Development)

if(Python3_FOUND)
   # PyCXX
   # Debian: python3-cxx-dev
   pkg_check_modules(PYCXX PyCXX)
endif()

# Apache Qpid Proton
# Debian: libqpid-proton-cpp12-dev
pkg_check_modules(QPIDPROTON libqpid-proton-cpp)

# gstreamermm
# Debian: libgstreamermm-1.0-dev
pkg_check_modules(GSTREAMERMM gstreamermm-1.0)

# websocketpp
# Debian: libwebsocketpp-dev
find_package(websocketpp)
if(websocketpp_FOUND)
   include_directories(SYSTEM ${websocketpp_INCLUDE_DIRS})
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

if(BERKELEYDB_FOUND AND CAJUN_FOUND)
   add_subdirectory(repro)
else()
   message(STATUS "Missing libraries, will not build repro")
endif()

# FIXME - need Netxx in CMake
#add_subdirectory(tfm)

add_subdirectory(media)

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

if(BUILD_P2P)
   add_subdirectory(p2p)
endif()

add_subdirectory(apps)

# Add 'make dist' command for creating release tarball
set (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set (CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.git/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeFiles/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/_CPack_Packages/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.deps/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.libs/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/*\\\\.gz")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/*\\\\.zip")
list(APPEND CPACK_SOURCE_IGNORE_FILES "*\\\\.o")
list(APPEND CPACK_SOURCE_IGNORE_FILES "lib*\\\\.so*")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeCache.txt")

include (CPack)
add_custom_target (dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

###############
### Summary ###
###############

include(FeatureSummary)
feature_summary(WHAT ALL)
