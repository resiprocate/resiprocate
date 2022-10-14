cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules")

if(POLICY CMP0042)
  CMAKE_POLICY(SET CMP0042 NEW)
endif(POLICY CMP0042)
if(POLICY CMP0077)
   CMAKE_POLICY(SET CMP0077 NEW)
endif(POLICY CMP0077)

# FIXME - how to include suffix such as 1.13.0~alpha1 ?
# https://cmake.org/cmake/help/latest/variable/PROJECT_VERSION.html
project(resiprocate VERSION 1.13.0)

include(GNUInstallDirs)
include(Utilities)

enable_testing()

option(ENABLE_LOG_REPOSITORY_DETAILS "Log repository revision and branch" TRUE)
if(ENABLE_LOG_REPOSITORY_DETAILS)
   execute_process(
      COMMAND git describe --match="" --always --abbrev=40 --dirty
      OUTPUT_VARIABLE RESIPROCATE_GIT_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      OUTPUT_VARIABLE RESIPROCATE_BRANCH_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# https://cmake.org/cmake/help/latest/module/FindThreads.html
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)
if(NOT Threads_FOUND)
   exit()
endif()
if(CMAKE_USE_PTHREADS_INIT)
   add_compile_definitions(_REENTRANT)
   add_compile_options(-pthread)
endif()

set(REPRO_BUILD_REV ${PACKAGE_VERSION})
set(REPRO_RELEASE_VERSION ${PACKAGE_VERSION})
set(RESIP_SIP_MSG_MAX_BYTES 10485760)

# https://cmake.org/cmake/help/latest/module/TestBigEndian.html
# see also
# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_BYTE_ORDER.html
include (TestBigEndian)
test_big_endian(RESIP_BIG_ENDIAN)

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
option(WITH_C_ARES "Link against libc-ares (rather than rutil/dns/ares)" FALSE)
option(WITH_SSL "Link against SSL libraries" TRUE)
#option(USE_POPT
option(USE_SIGCOMP "Use OpenSigComp" FALSE)
#option(USE_FMT
option(VERSIONED_SONAME "Include Major.Minor version in SONAME" TRUE)
option(ENABLE_ANDROID "Enable Android build" FALSE)
option(USE_IPV6 "Enable IPv6" TRUE)
option(USE_DTLS "Enable DTLS" TRUE)
option(PEDANTIC_STACK "Enable pedantic behavior (fully parse all messages)" FALSE)
# some systems may have a newer version of libpq that is not
# compatible with the packaged version of soci_postgresql
option(USE_SOCI_POSTGRESQL "Build recon with SOCI PostgreSQL support" FALSE)
# can't have both MariaDB C client and SOCI MySQL at the same
# time in some environments, e.g. CentOS 8.1
option(USE_SOCI_MYSQL "Build recon with SOCI MySQL support" FALSE)
option(USE_POSTGRESQL "Link against PostgreSQL client libraries" FALSE)
option(USE_MAXMIND_GEOIP "Link against MaxMind GeoIP libraries" FALSE)
option(RESIP_HAVE_RADCLI "Link against radcli RADIUS client library" FALSE)
option(BUILD_REPRO "Build repro SIP proxy" TRUE)
option(BUILD_DSO_PLUGINS "Build DSO plugins" TRUE)
option(BUILD_RETURN "Build reTurn server" TRUE)
option(BUILD_REND "Build rend" FALSE)
option(BUILD_TFM "Build TFM, requires Netxx and cppunit" FALSE)
option(BUILD_ICHAT_GW "Build iChat gateway, requires gloox" FALSE)
option(BUILD_TELEPATHY_CM "Build Telepathy connection manager" FALSE)
option(BUILD_RECON "Build reCon Conversation Manager library" FALSE)
option(USE_SRTP1 "Use srtp 1.x instead of current version" FALSE)
option(BUILD_RECONSERVER "Build reConServer" FALSE)
option(USE_SIPXTAPI "Link against sipXtapi" FALSE)
option(USE_KURENTO "Build Kurento client (requires websocketpp)" FALSE)
option(USE_GSTREAMER "Link against Gstreamer" FALSE)
option(USE_LIBWEBRTC "Link against LibWebRTC" FALSE)
option(RECON_LOCAL_HW_TESTS "Attempt to use local audio hardware in unit tests" FALSE)
option(DEFAULT_BRIDGE_MAX_IN_OUTPUTS "recon: Maximum connections on bridge" 20)
option(BUILD_P2P "Build P2P, links against S2C and SSL, unfinished" FALSE)
option(BUILD_PYTHON "Build components requiring Python" FALSE)
option(BUILD_QPID_PROTON "Build components requiring qpid-proton (AMQP)" TRUE)
option(RESIP_ASSERT_SYSLOG "Log assertion failures with Syslog" TRUE)

if(WIN32)
   set(BUILD_SHARED_LIBS_DEFAULT OFF)
else()
   set(BUILD_SHARED_LIBS_DEFAULT ON)
endif()
option(BUILD_SHARED_LIBS "Build libraries as shared" ${BUILD_SHARED_LIBS_DEFAULT})

set(CMAKE_CXX_STANDARD 11)

####################
### Dependencies ###
####################

find_package(PkgConfig)

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

# OpenSigComp
if(USE_SIGCOMP)
   if(WIN32)
      add_subdirectory(contrib/opensigcomp)
      set(OPENSIGCOMP_LIBRARIES opensigcomp)
   else()
      find_package(OpenSigComp)
      include_directories(${OPENSIGCOMP_INCLUDE_DIRS})
   endif()
endif()

# fmt
# Debian: libfmt-dev
find_package(fmt)
if(fmt_FOUND)
endif()

if(NOT VERSIONED_SONAME)
   set(CMAKE_PLATFORM_NO_VERSIONED_SONAME True)
endif()

function(option_def)
   if(${ARGV0})
      add_definitions(-D${ARGV0})
   endif()
endfunction()

function(set_def)
   set(${ARGV0} TRUE)
   add_definitions(-D${ARGV0})
endfunction()

function(package_req)
   if(${ARGV0})
      find_package(${ARGV1} REQUIRED)
   endif()
endfunction()

option_def(USE_IPV6)
option_def(USE_DTLS)
option_def(PEDANTIC_STACK)

# MySQL
# Debian: default-libmysqlclient-dev
find_package(MySQL)
package_req(USE_MYSQL MySQL)

# soci (MySQL, PostgreSQL)
# Debian: libsoci-dev
# FIXME - hardcoded
# FIXME - MySQL, PostgreSQL)
if(USE_SOCI_POSTGRESQL OR USE_SOCI_MYSQL)
   set(SOCI_FOUND true)
   set(SOCI_INCLUDE_DIRS "/usr/include/soci")
   set(SOCI_LIBRARIES soci_core)
   option_def(USE_SOCI_POSTGRESQL)
   option_def(USE_SOCI_MYSQL)
endif()

# PostgreSQL
# Debian: libpq-dev postgresql-server-dev-all
package_req(USE_POSTGRESQL PostgreSQL)

# GeoIP
# Debian: libgeoip-dev
package_req(USE_MAXMIND_GEOIP GeoIP)

# radcli (RADIUS client)
# Debian: libradcli-dev
if(RESIP_HAVE_RADCLI)
   pkg_check_modules(LIBRADIUS radcli REQUIRED)
   option_def(RESIP_HAVE_RADCLI)
   set_def(USE_RADIUS_CLIENT)
endif()

# NetSNMP
# Debian: libsnmp-dev
pkg_check_modules(NETSNMP_AGENT netsnmp-agent)
if(NETSNMP_AGENT_FOUND)
   set_def(USE_NETSNMP)
endif()

option_def(BUILD_REPRO)

if(BUILD_DSO_PLUGINS)
   add_definitions(-DDSO_PLUGINS)
   get_filename_component(INSTALL_REPRO_PLUGIN_DIR ${CMAKE_INSTALL_PKGLIBDIR}/repro/plugins ABSOLUTE)
endif()

get_filename_component(INSTALL_RETURN_PKGLIB_DIR ${CMAKE_INSTALL_PKGLIBDIR}/reTurn} ABSOLUTE)

option_def(BUILD_RETURN)

option_def(BUILD_REND)

option_def(BUILD_TFM)

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

# SRTP2
# Debian: libsrtp2-dev
if(USE_SRTP1)
   find_package(srtp)
   if(SRTP_FOUND)
      include_directories(SYSTEM ${SRTP_INCLUDE_DIRS})
   endif()
else()
   find_package(srtp2)
   if(SRTP2_FOUND)
      include_directories(SYSTEM ${SRTP2_INCLUDE_DIRS})
   endif()
endif()

# sipXtapi
# Debian: libsipxtapi-dev
# FIXME - hardcoded
if(USE_SIPXTAPI)
   option_def(USE_SIPXTAPI)
   set(SIPXTAPI_FOUND true)
   if(WIN32)
      # FIXME
      set(SIPXTAPI_LIBRARIES sipXport sipXsdp sipXmedia sipXmediaProcessing)
   else()
      set(SIPXTAPI_INCLUDE_DIRS "/usr/include/sipxtapi")
      set(SIPXTAPI_LIBRARIES sipXport sipXsdp sipXmedia sipXmediaProcessing)
      set(SIPX_NO_RECORD true)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__pingtel_on_posix__")
   endif()
endif()

option_def(USE_KURENTO)

# gstreamermm
# Debian: libgstreamermm-1.0-dev
if(USE_GSTREAMER)
   option_def(USE_GSTREAMER)
   pkg_check_modules(GSTREAMERMM gstreamermm-1.0 REQUIRED)

   # gstwebrtc-1.0
   # Debian: libgstreamer-plugins-bad1.0-dev
   pkg_check_modules(GST_WEBRTC gstreamer-webrtc-1.0)
endif()

option_def(USE_LIBWEBRTC)

option_def(RECON_LOCAL_HW_TESTS)

# Python
# Debian: python3-dev
if(BUILD_PYTHON)
   find_package(Python3 COMPONENTS Development REQUIRED)
   if(Python3_FOUND)
      # PyCXX
      # Debian: python3-cxx-dev
      pkg_check_modules(PYCXX PyCXX)
   endif()
endif()

# Apache Qpid Proton
# Debian: libqpid-proton-cpp12-dev
if(BUILD_QPID_PROTON)
   pkg_check_modules(QPIDPROTON libqpid-proton-cpp REQUIRED)
   option_def(BUILD_QPID_PROTON)
endif()

option_def(RESIP_ASSERT_SYSLOG)

# websocketpp
# Debian: libwebsocketpp-dev
find_package(websocketpp)
if(websocketpp_FOUND)
   include_directories(SYSTEM ${websocketpp_INCLUDE_DIRS})
endif()

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

# epoll
include(CheckIncludeFiles)
check_include_files(sys/epoll.h HAVE_EPOLL)

# gperf
set(GPERF_SIZE_TYPE "size_t")

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

if(BUILD_REPRO)
   add_subdirectory(repro)
endif()

if(BUILD_TFM)
   add_subdirectory(tfm)
endif()

add_subdirectory(media)

if(BUILD_RETURN)
   add_subdirectory(reTurn)
endif()

if(BUILD_RECON)
   add_subdirectory(reflow)
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
