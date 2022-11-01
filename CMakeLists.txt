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

# Warning: CMAKE_BUILD_TYPE
#
# Using CMAKE_BUILD_TYPE=Debug will create a build without the NDEBUG flag
#
# Any other CMAKE_BUILD_TYPE appears to set NDEBUG sometimes
# Some documentation suggest that CMAKE_BUILD_TYPE=RelWithDebInfo will
# not set the NDEBUG flag but this appears to be inconsistent.
#
# reSIProcate makes extensive use of assert()
# assert() is disabled by NDEBUG
# Application code should not rely on NDEBUG for any purpose, its only
# purpose is for disabling assert()
#
# https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html#variable:CMAKE_BUILD_TYPE
#

# shared library versioning

#set(SO_ABI "0.0.0")
set(SO_RELEASE "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

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
find_package(Threads REQUIRED)
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
option(USE_POPT "Link against POPT libraries" FALSE)
option(USE_SIGCOMP "Use OpenSigComp" FALSE)
option(USE_FMT "Link against fmt library" FALSE)
option(VERSIONED_SONAME "Include Major.Minor version in SONAME" TRUE)
option(ENABLE_ANDROID "Enable Android build" FALSE)
option(USE_IPV6 "Enable IPv6" TRUE)
option(USE_DTLS "Enable DTLS" TRUE)
option(PEDANTIC_STACK "Enable pedantic behavior (fully parse all messages)" FALSE)
option(USE_MYSQL "Link against MySQL client libraries" FALSE)
# some systems may have a newer version of libpq that is not
# compatible with the packaged version of soci_postgresql
option(USE_SOCI_POSTGRESQL "Build recon with SOCI PostgreSQL support" FALSE)
# can't have both MariaDB C client and SOCI MySQL at the same
# time in some environments, e.g. CentOS 8.1
option(USE_SOCI_MYSQL "Build recon with SOCI MySQL support" FALSE)
option(USE_POSTGRESQL "Link against PostgreSQL client libraries" FALSE)
option(USE_MAXMIND_GEOIP "Link against MaxMind GeoIP libraries" FALSE)
option(RESIP_HAVE_RADCLI "Link against radcli RADIUS client library" FALSE)
option(USE_NETSNMP "Link against NetSNMP client libraries" FALSE)
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

#
# Libtool / autotools is able to build both the static and shared
# version of a library based on a single definition of the library.
#
# CMake is trying to support platforms like Windows that do not
# allow both the static and shared library to share the same target
# name.
#
# Therefore, with our initial CMake implementation, we only support
# one type of build or the other.
#
# People who need both static and shared libraries can run the build
# twice with alternate values of BUILD_SHARED_LIBS
#
# FIXME - can we replicate one of the hacks for building both static
#         and shared?
#       - it is a good idea to ask the CMake developers and ask the
#         packaging system maintainers (debhelper, rpmbuild)
#
# Example hacks:
#  https://stackoverflow.com/questions/2152077/is-it-possible-to-get-cmake-to-build-both-a-static-and-shared-library-at-the-sam
#  https://github.com/baresip/rem/pull/84/files
#
if(WIN32)
   set(BUILD_SHARED_LIBS_DEFAULT OFF)
   set(USE_CONTRIB_DEFAULT ON)
else()
   set(BUILD_SHARED_LIBS_DEFAULT ON)
   set(USE_CONTRIB_DEFAULT OFF)
endif()
option(BUILD_SHARED_LIBS "Build libraries as shared" ${BUILD_SHARED_LIBS_DEFAULT})
option(USE_CONTRIB "Use libraries from contrib folder" ${USE_CONTRIB_DEFAULT})

set(CMAKE_CXX_STANDARD 11)

########################
### Helper functions ###
########################

function(option_def)
   if(${ARGV0})
      add_definitions(-D${ARGV0})
   endif()
endfunction()

function(set_def)
   set(${ARGV0} TRUE)
   add_definitions(-D${ARGV0})
endfunction()

function(do_fail_win32)
   message(FATAL_ERROR "please complete Win32 support for ${ARGV0} in CMakeLists.txt")
endfunction()

# See
#   https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
#   https://cmake.org/cmake/help/latest/prop_tgt/VERSION.html
function(version_libname)
   if(SO_ABI)
      set_target_properties(${ARGV0} PROPERTIES SOVERSION ${SO_ABI})
   endif()
   # This logic tries to replicate the libtool -release X.Y ...
   # but it doesn't create the same symlink that libtool creates.
   # FIXME
   # Other people have complained about the same problem, e.g.
   # https://discourse.libsdl.org/t/patches-dynamic-library-name-should-it-be-libsdl2-2-0-so-or-libsdl2-so/19400/8
   if(VERSIONED_SONAME)
      set_target_properties(${ARGV0} PROPERTIES OUTPUT_NAME ${ARGV0}-${SO_RELEASE})
   endif()
endfunction()

################################
### Per-program dependencies ###
################################

if(BUILD_REPRO)
   set(USE_BDB TRUE)
   set(USE_PCRE TRUE)
   set(USE_CAJUN TRUE)
endif()

if(BUILD_RECON)
   set(USE_SRTP TRUE)
endif()

if(BUILD_TFM)
   set(USE_NETXX TRUE)
   set(USE_CPPUNIT TRUE)
endif()

if(BUILD_RETURN)
   set(USE_ASIO TRUE)
   set(USE_BOOST TRUE)
endif()

if(USE_KURENTO)
   set(USE_ASIO TRUE)
   set(USE_WEBSOCKETPP TRUE)
endif()


####################
### Dependencies ###
####################

find_package(PkgConfig REQUIRED)

# ares
if(WITH_C_ARES)
   # Don't use built-in ares
   pkg_check_modules(cares libcares REQUIRED)

   set(USE_CARES true)
   add_definitions(-DUSE_CARES)

   set(ARES_LIBRARIES ${cares_LIBRARIES})
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
   set_def(USE_SSL)
else()
   # Explicitly set OPENSSL_FOUND to false since we didn't even run
   # find_package on it. It needs to be set to false for other CMake scripts to
   # know it is not being used.
   set(OPENSSL_FOUND FALSE)
endif()

# popt
if(USE_POPT)
   # Debian: libpopt-dev
   if(USE_CONTRIB)
      add_subdirectory(contrib/popt)
      set(POPT_LIBRARIES popt)
      set(POPT_INCLUDE_DIRS contrib/popt)
   else()
      find_package(popt REQUIRED)
   endif()
   set(HAVE_POPT_H true)
   add_definitions(-DHAVE_POPT_H)
endif()

# OpenSigComp
if(USE_SIGCOMP)
   if(USE_CONTRIB)
      add_subdirectory(contrib/opensigcomp)
      set(opensigcomp_LIBRARIES opensigcomp)
   else()
      find_package(opensigcomp REQUIRED)
   endif()
endif()

# fmt
# Debian: libfmt-dev
if(USE_FMT)
   if(USE_CONTRIB)
      add_subdirectory(contrib/fmt)
   else()
      find_package(fmt REQUIRED)
   endif()
   set_def(USE_FMT)
endif()

if(NOT VERSIONED_SONAME)
   set(CMAKE_PLATFORM_NO_VERSIONED_SONAME True)
endif()

option_def(USE_IPV6)
option_def(USE_DTLS)
option_def(PEDANTIC_STACK)

# MySQL
# Debian: default-libmysqlclient-dev
if(USE_MYSQL)
   if(USE_CONTRIB)
      add_subdirectory(contrib/MySQLConnectorC)
   else()
      find_package(MySQL REQUIRED)
   endif()
endif()

# soci (MySQL, PostgreSQL)
# Debian: libsoci-dev
# FIXME - hardcoded
# FIXME - MySQL, PostgreSQL
if(USE_SOCI_POSTGRESQL OR USE_SOCI_MYSQL)
   find_library(SOCI_LIBRARIES soci_core REQUIRED)
   # FIXME include path
   set(SOCI_INCLUDE_DIRS "/usr/include/soci")
   option_def(USE_SOCI_POSTGRESQL)
   option_def(USE_SOCI_MYSQL)
endif()

# PostgreSQL
# Debian: libpq-dev postgresql-server-dev-all
if(USE_POSTGRESQL)
   if(USE_CONTRIB)
      add_subdirectory(contrib/psql)
   else()
      find_package(PostgreSQL REQUIRED)
   endif()
endif()

# GeoIP
# Debian: libgeoip-dev
if(USE_MAXMIND_GEOIP)
   if(USE_CONTRIB)
      add_subdirectory(contrib/GeoIP)
   else()
      find_package(GeoIP REQUIRED)
   endif()
endif()

# radcli (RADIUS client)
# Debian: libradcli-dev
if(RESIP_HAVE_RADCLI)
   if(USE_CONTRIB)
      do_fail_win32("radcli")
   else()
      pkg_check_modules(LIBRADIUS radcli REQUIRED)
   endif()
   option_def(RESIP_HAVE_RADCLI)
   set_def(USE_RADIUS_CLIENT)
endif()

# NetSNMP
# Debian: libsnmp-dev
if(USE_NETSNMP)
   if(USE_CONTRIB)
      do_fail_win32("netsnmp")
   else()
      pkg_check_modules(NETSNMP_AGENT netsnmp-agent REQUIRED)
   endif()
   set_def(USE_NETSNMP)
endif()

option_def(BUILD_REPRO)

set(CMAKE_INSTALL_PKGLIBDIR ${CMAKE_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME})
set(CMAKE_INSTALL_MIBDIR ${CMAKE_INSTALL_DATAROOTDIR}/snmp/mibs)

if(BUILD_DSO_PLUGINS)
   add_definitions(-DDSO_PLUGINS)
   set(INSTALL_REPRO_PLUGIN_DIR ${CMAKE_INSTALL_PKGLIBDIR}/repro/plugins)
endif()

set(INSTALL_RETURN_PKGLIB_DIR ${CMAKE_INSTALL_PKGLIBDIR}/reTurnServer)

option_def(BUILD_RETURN)

option_def(BUILD_REND)

option_def(BUILD_TFM)

# Netxx
# Debian: libnetxx-dev
if(USE_NETXX)
   if(USE_CONTRIB)
      add_subdirectory(contrib/Netxx-0.3.2)
      set(Netxx_LIBRARIES Netxx)
   else()
      find_package(Netxx REQUIRED)
   endif()
endif()

# cppunit
# Debian: libcppunit-dev
if(USE_CPPUNIT)
   if(USE_CONTRIB)
      add_subdirectory(contrib/cppunit)
      set(CPPUNIT_LIBRARIES cppunit)
   else()
      pkg_check_modules(CPPUNIT cppunit REQUIRED)
   endif()
endif()

# BerkeleyDb
# Debian: libdb++-dev
if(USE_BDB)
   if(USE_CONTRIB)
      add_subdirectory(contrib/db)
   else()
      find_package(BerkeleyDb REQUIRED)
   endif()
   set(DB_HEADER "db_cxx.h")
endif()

# PCRE
# Debian: libpcre3-dev
if(USE_PCRE)
  if(USE_CONTRIB)
    add_subdirectory(contrib/pcre)
    set(PCRE_LIBRARIES pcre)
  else()
    pkg_check_modules(PCRE REQUIRED libpcre)
  endif()
endif()

# Cajun
# Debian: libcajun-dev
#set(CAJUN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cajun/include")
if(USE_CAJUN)
   if(USE_CONTRIB)
      add_subdirectory(contrib/cajun)
   else()
      find_package(cajun REQUIRED)
   endif()
endif()

# ASIO
# Debian: libasio-dev
# modern ASIO requires C++11
if(USE_ASIO)
   if(USE_CONTRIB)
      set(ASIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/asio")
   else()
      find_package(ASIO REQUIRED)
   endif()
endif()

# Boost
# Debian: libboost-all-dev
if(USE_BOOST)
   find_package(Boost REQUIRED)
endif()

if(BUILD_TELEPATHY_CM)
   set(USE_QT5)
endif()

# Telepathy-Qt
# Debian: libtelepathy-qt5-dev
if(USE_QT5)
   find_package(TelepathyQt5 REQUIRED)
endif()

# SRTP2
# Debian: libsrtp2-dev
if(USE_SRTP)
   if(USE_SRTP1)
      find_package(srtp REQUIRED)
   else()
      find_package(srtp2 REQUIRED)
   endif()
endif()

# sipXtapi
# Debian: libsipxtapi-dev
if(USE_SIPXTAPI)
   option_def(USE_SIPXTAPI)
   find_package(sipXtapi REQUIRED)
   if(NOT WIN32)
      set(SIPX_NO_RECORD true)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__pingtel_on_posix__")
   endif()
endif()

option_def(USE_KURENTO)

# gstreamermm
# Debian: libgstreamermm-1.0-dev
if(USE_GSTREAMER)
   option_def(USE_GSTREAMER)
   pkg_check_modules(GSTREAMERMM_1_0 gstreamermm-1.0 REQUIRED)

   # gstwebrtc-1.0
   # Debian: libgstreamer-plugins-bad1.0-dev
   pkg_check_modules(GST_WEBRTC gstreamer-webrtc-1.0 REQUIRED)
endif()

option_def(USE_LIBWEBRTC)

option_def(RECON_LOCAL_HW_TESTS)

# Python
# Debian: python3-dev python3-cxx-dev
if(BUILD_PYTHON)
   find_package(Python3 COMPONENTS Development REQUIRED)
   pkg_check_modules(PYCXX PyCXX REQUIRED)
   if(NOT PYCXX_SRCDIR)
      pkg_get_variable(PYCXX_SRCDIR PyCXX srcdir)
      if(NOT PYCXX_SRCDIR)
         message(FATAL_ERROR "Failed to obtain PyCXX srcdir automatically, please set it manually or disable BUILD_PYTHON")
      endif()
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
if(USE_WEBSOCKETPP)
   find_package(websocketpp REQUIRED)
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
list(APPEND CPACK_SOURCE_IGNORE_FILES "/.*\\\\.gz")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/.*\\\\.zip")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".*\\\\.o")
list(APPEND CPACK_SOURCE_IGNORE_FILES "lib.*\\\\.so*")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeCache.txt")

include (CPack)
add_custom_target (dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

###############
### Summary ###
###############

include(FeatureSummary)
feature_summary(WHAT ALL)
