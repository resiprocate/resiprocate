cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

option(USE_VCPKG "assume vcpkg presence and include it so find_package finds it." FALSE)
if(USE_VCPKG)
  include(${VCPKG_DIR})
# include("C:/On-Hertz/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
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
include(CheckStructHasMember)

enable_testing()

option(ENABLE_LOG_REPOSITORY_DETAILS "Log repository revision and branch" TRUE)
if(ENABLE_LOG_REPOSITORY_DETAILS)
   find_package(Git REQUIRED)
   execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --match="" --always --abbrev=40 --dirty
      OUTPUT_VARIABLE RESIPROCATE_GIT_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
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
   add_definitions(-D__REENTRANT)
   add_definitions(-pthread)
endif()

set(REPRO_BUILD_REV ${PACKAGE_VERSION})
set(REPRO_RELEASE_VERSION ${PACKAGE_VERSION})
set(RESIP_SIP_MSG_MAX_BYTES 10485760)

# https://cmake.org/cmake/help/latest/module/TestBigEndian.html
# see also
# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_BYTE_ORDER.html
include (TestBigEndian)
test_big_endian(RESIP_BIG_ENDIAN)

CHECK_STRUCT_HAS_MEMBER(sockaddr_in sin_len arpa/inet.h HAVE_sockaddr_in_len)
if(HAVE_sockaddr_in_len)
   add_definitions(-DHAVE_sockaddr_in_len)
endif()

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
option(BUILD_CLICKTOCALL "Build Click to call application" FALSE)
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
option(BUILD_P2P "Build P2P, links against S2C and SSL, unfinished" FALSE)
option(BUILD_PYTHON "Build components requiring Python" FALSE)
option(BUILD_QPID_PROTON "Build components requiring qpid-proton (AMQP)" TRUE)
option(RESIP_ASSERT_SYSLOG "Log assertion failures with Syslog" TRUE)
option(REGENERATE_MEDIA_SAMPLES "Regenerate the header files containing raw audio samples (requires sox, xxd)" TRUE)

set(DEFAULT_BRIDGE_MAX_IN_OUTPUTS 20 CACHE STRING "recon: Maximum connections on bridge")

# This must be enabled when building with the Android ndkports tools.
# It should not be enabled for any other case.
option(USE_NDKPORTS_HACKS "Android ndkports build: use hardcoded paths to dependencies" FALSE)

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
   set(USE_NUGET_DEFAULT ON)
else()
   set(BUILD_SHARED_LIBS_DEFAULT ON)
   set(USE_CONTRIB_DEFAULT OFF)
   set(USE_NUGET_DEFAULT OFF)
endif()
option(BUILD_SHARED_LIBS "Build libraries as shared" ${BUILD_SHARED_LIBS_DEFAULT})
option(USE_CONTRIB "Use libraries from contrib folder" ${USE_CONTRIB_DEFAULT})
option(USE_NUGET "Use NuGet package manager" ${USE_NUGET_DEFAULT})

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
      file(CREATE_LINK lib${ARGV0}-${SO_RELEASE}.so ${CMAKE_CURRENT_BINARY_DIR}/lib${ARGV0}.so RESULT ${ARGV0}-IGNORE SYMBOLIC)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${ARGV0}.so DESTINATION ${CMAKE_INSTALL_LIBDIR})
   endif()
endfunction()

if(NOT VERSIONED_SONAME)
   set(CMAKE_PLATFORM_NO_VERSIONED_SONAME True)
endif()

################################
### Per-program dependencies ###
################################

set(USE_PCRE FALSE)
set(USE_WEBSOCKETPP FALSE)

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
   #set(USE_BOOST TRUE)  # Dependency has been replaced by C++11
endif()

if(USE_KURENTO)
   set(USE_ASIO TRUE)
   set(USE_WEBSOCKETPP TRUE)
endif()

if(BUILD_CLICKTOCALL)
   set(USE_PCRE TRUE)
endif()

####################
### Dependencies ###
####################

if(NOT USE_CONTRIB)
   find_package(PkgConfig REQUIRED)
endif()

# ares
if(WITH_C_ARES)
   # Don't use built-in ares
   pkg_check_modules(cares libcares REQUIRED)

   set(USE_CARES true)
   add_definitions(-DUSE_CARES)

   set(ARES_LIBRARIES ${cares_LIBRARIES})
   include_directories(${cares_INCLUDE_DIRS})
   link_directories(${cares_LIBRARY_DIRS})
else()
   # Use built-in ares
   set(USE_ARES true)
   add_definitions(-DUSE_ARES)

   # Put the resip ares include dir before the system ones to not conflict with
   # c-ares if also present.
   include_directories(BEFORE rutil/dns/ares)
   
   set(ARES_LIBRARIES resipares)
endif()

if("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
   set(WIN_ARCH "x64")
else()
   set(WIN_ARCH "Win32")
endif()

# Download NuGet Package Manager and ensure it is in %PATH%
# before running CMake
if(USE_NUGET)
   find_program(NUGET_EXE NAMES nuget.exe REQUIRED)
endif()

function(nuget_inst)
   exec_program(${NUGET_EXE}
      ARGS install "${ARGV1}" -Version ${ARGV2} -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages
      )
   set(${ARGV0}_TARGETS ${CMAKE_BINARY_DIR}/packages/${ARGV1}/build/native/${ARGV1}.targets PARENT_SCOPE)

   set(PKG_TOP "${CMAKE_BINARY_DIR}/packages/${ARGV1}/build/native")
   set(PKG_LIB "${PKG_TOP}/lib/${WIN_ARCH}/Debug")  # FIXME

   set(${ARGV0}_ROOT_DIR "${PKG_TOP}" PARENT_SCOPE)

   # These vary a lot from one package to the next.  We
   # set them to some common default values but in many
   # cases the subsequent code needs to override these
   # values with package-specific values.
   set(${ARGV0}_INCLUDE_DIRS "${PKG_TOP}/include" PARENT_SCOPE)
   set(${ARGV0}_LIBRARIES_DIR "${PKG_LIB}" PARENT_SCOPE)
endfunction()

# OpenSSL
if(WITH_SSL)
   if(USE_NUGET)
      nuget_inst(OPENSSL "zeroc.openssl.v142" "1.1.1.2")
      message("Using ${OPENSSL_ROOT_DIR} and ${OPENSSL_LIBRARIES}")
      # this is an argument to FindOpenSSL
      # https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
      set(OPENSSL_USE_STATIC_LIBS True)
#      set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_LIBRARIES}/libcrypto.lib")
      set(OPENSSL_LIBRARIES
            "${OPENSSL_LIBRARIES_DIR}/libssl.lib"
            "${OPENSSL_LIBRARIES_DIR}/libcrypto.lib")
      # FIXME - check if those libraries and headers really exist
      set(OPENSSL_FOUND TRUE)
   elseif(USE_NDKPORTS_HACKS)
      if(NOT ANDROID_ABI)
         message(FATAL_ERROR "ndkports hack requested but ANDROID_ABI not set")
      endif()
      #set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../distribution)
      #set(OPENSSL_ROOT_DIR ${distribution_DIR}/openssl/${ANDROID_ABI})
      set(OPENSSL_ROOT_DIR /src/openssl/build/port/aar/prefab/modules)

      set(CRYPTO_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/crypto/libs/android.${ANDROID_ABI}")
      set(SSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/ssl/libs/android.${ANDROID_ABI}")
      set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/ssl/include)
      set(OPENSSL_LIBRARIES
            "${SSL_LIBRARIES_DIR}/libssl.so"
            "${CRYPTO_LIBRARIES_DIR}/libcrypto.so")
      # FIXME - check if those libraries and headers really exist
      set(OPENSSL_FOUND TRUE)
   else()
      # SSL support is requested, so make it mandatory when calling find_package
      find_package(OpenSSL REQUIRED) # HINTS ${OPENSSL_LIBRARIES})
   endif()
   # Oldest OpenSSL API to target (1.1.1)
   add_compile_definitions(OPENSSL_API_COMPAT=0x10101000L)
   set_def(USE_SSL)
else()
   # Explicitly set OPENSSL_FOUND to false since we didn't even run
   # find_package on it. It needs to be set to false for other CMake scripts to
   # know it is not being used.
   set(OPENSSL_FOUND FALSE)
endif()

# popt
# Debian: libpopt-dev
if(USE_POPT)
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

option_def(USE_IPV6)
option_def(USE_DTLS)
option_def(PEDANTIC_STACK)

# MySQL
# Debian: default-libmysqlclient-dev
if(USE_MYSQL)
   if(USE_CONTRIB)
      # There is a pre-compiled MySQL client binary in contrib
      # so we hardcode the paths to the headers and binary
      # artifacts.
      #add_subdirectory(contrib/MySQLConnectorC)
      set(MySQL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/contrib/MySQLConnectorC")
      set(MySQL_INCLUDE_DIRS "${MySQL_ROOT}/include")
      set(MySQL_LIBRARIES "${MySQL_ROOT}/lib/debug/libmysql.lib")
      set(MySQL_FOUND TRUE)
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
      # FIXME - update reSIProcate for libmaxminddb - successor to GeoIP
      message(FATAL_ERROR "GeoIP has been deprecated upstream, see https://github.com/maxmind/geoip-api-c")
      #add_subdirectory(contrib/GeoIP)
   else()
      find_package(GeoIP REQUIRED)
   endif()
endif()

# radcli (RADIUS client)
# Debian: libradcli-dev
# FIXME: do we need to support alternatives like
# freeradius-client and radiusclient-ng?
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
      # net-snmp-config --agent-libs
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

# BUILD_APPS has been omitted

option_def(BUILD_ICHAT_GW)

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
   if(USE_NUGET)
      #nuget_inst(BERKELEYDB "BerkeleyDB-4_8_30-Cpp" "4.8.30.2")
      #set(BERKELEYDB_LIBRARIES_DIR "${BERKELEYDB_ROOT_DIR}/bin")
      #set(BERKELEYDB_LIBRARIES
      #      "${BERKELEYDB_LIBRARIES_DIR}/lib/${WIN_ARCH}/Debug/libdb48d.lib")
      # https://www.nuget.org/packages/berkeley.db.v140
      nuget_inst(BERKELEYDB "berkeley.db.v140" "5.3.28.3")
      set(BERKELEYDB_LIBRARIES_DIR "${BERKELEYDB_ROOT_DIR}/lib/${WIN_ARCH}/Debug")
      set(BERKELEYDB_LIBRARIES
            "${BERKELEYDB_LIBRARIES_DIR}/libdb53d.lib")
      # FIXME - check if those libraries and headers really exist
      set(BERKELEYDB_FOUND TRUE)
   elseif(USE_CONTRIB)
      add_subdirectory(contrib/db)
   else()
      find_package(BerkeleyDb REQUIRED)
   endif()
   set(DB_HEADER "db_cxx.h")
endif()

# PCRE
# Debian: libpcre3-dev
# FIXME - deprecate PCRE in favour of C++11 <regex>
if(USE_PCRE)
   if(USE_NUGET)
      nuget_inst(PCRE "pcre" "8.33.0.1")
      if(BUILD_SHARED_LIBS)
         set(WIN_LIB_LINKING "dynamic")
      else()
         set(WIN_LIB_LINKING "static")
         add_definitions(-DPCRE_STATIC)
      endif()
      set(PCRE_LIBRARIES_DIR "${PCRE_ROOT_DIR}/lib/v110/${WIN_ARCH}/Debug/${WIN_LIB_LINKING}/utf8")
      set(PCRE_LIBRARIES
              "${PCRE_LIBRARIES_DIR}/pcre8.lib")
      # FIXME - check if those libraries and headers really exist
      if(NOT EXISTS "${PCRE_LIBRARIES}")
         message(FATAL_ERROR "could not find ${PCRE_LIBRARIES}")
      endif()
      set(PCRE_FOUND TRUE)
   elseif(USE_CONTRIB)
      add_subdirectory(contrib/pcre)
      set(PCRE_LIBRARIES pcre)
   else()
      pkg_check_modules(PCRE REQUIRED libpcre)
   endif()
endif()

# Cajun
# Debian: libcajun-dev
if(USE_CAJUN)
   if(USE_CONTRIB)
      # FIXME: convert Cajun to CMake build system
      # FIXME: include Cajun as a Git submodule or NuGet
      #add_subdirectory(contrib/cajun)
      set(CAJUN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cajun/include")
   else()
      find_package(cajun REQUIRED)
   endif()
endif()

# ASIO
# Debian: libasio-dev
# modern ASIO requires C++11
if(USE_ASIO)
   if(USE_CONTRIB)
      # FIXME: include asio as a Git submodule or NuGet
      set(ASIO_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/asio")
   else()
      find_package(ASIO REQUIRED)
   endif()
endif()

# Boost
# Debian: libboost-all-dev
if(USE_BOOST)
   if(USE_NUGET)
      nuget_inst(BOOST "boost" "1.80.0")
      message("Using ${BOOST_ROOT_DIR} and ${BOOST_LIBRARIES}")
      set(BOOST_LIBRARIES
            "${BOOST_LIBRARIES_DIR}/libboostcore.lib")
      # FIXME - check if those libraries and headers really exist
      set(BOOST_FOUND TRUE)
   else()
      find_package(Boost REQUIRED)
   endif()
endif()

if(BUILD_TELEPATHY_CM)
   option_def(BUILD_TELEPATHY_CM)
   set(USE_QT5)
endif()

# Telepathy-Qt
# Debian: libtelepathy-qt5-dev
if(USE_QT5)
   # PKG_CHECK_MODULES([QT5], [Qt5Core, Qt5DBus Qt5Network])
   # PKG_CHECK_MODULES([TP_QT5], [TelepathyQt5, TelepathyQt5Service])])
   find_package(TelepathyQt5 REQUIRED)
endif()

option_def(BUILD_RECON)

# SRTP2
# Debian: libsrtp2-dev
if(USE_SRTP)
   if(USE_SRTP1)
      find_package(srtp REQUIRED)
   else()
      # FIXME - not in NuGet, not CMake native
      if(USE_CONTRIB)
         add_subdirectory("contrib/srtp")
         #set(libsrtp2_INCLUDE_DIRS "${libsrtp2_INCLUDE_DIRS}")
         #set(libsrtp2_LIBRARIES "${libsrtp2_LIBRARIES}")
         # FIXME - these shouldn't be hard-coded, need to fix the above variables
         set(SRTP2_INCLUDE_DIRS "${libsrtp2_SOURCE_DIR}/include")
         set(SRTP2_LIBRARIES "${libsrtp2_BINARY_DIR}/contrib/srtp/${CMAKE_BUILD_TYPE}/srtp2.lib")
         set(SRTP_FOUND TRUE)
      else()
         find_package(srtp2 REQUIRED)
      endif()
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
   add_definitions(-DPy_LIMITED_API=0x03090000)
endif()

# Apache Qpid Proton
# Debian: libqpid-proton-cpp12-dev
if(BUILD_QPID_PROTON)
   if(USE_NUGET)
      # FIXME - they only publish the C API, need full C++ API
      nuget_inst(QPIDPROTON "Apache.Qpid.Proton" "1.0.0-M6")
      message("Using ${QPIDPROTON_ROOT_DIR} and ${QPIDPROTON_LIBRARIES}")
      set(QPIDPROTON_LIBRARIES
            "${QPIDPROTON_LIBRARIES_DIR}/libqpidproton.lib")
      # FIXME - check if those libraries and headers really exist
      set(QPIDPROTON_FOUND TRUE)
   else()
      pkg_check_modules(QPIDPROTON libqpid-proton-cpp REQUIRED)
   endif()
   option_def(BUILD_QPID_PROTON)
endif()

option_def(RESIP_ASSERT_SYSLOG)

# FIXME
# The AC_SEARCH_LIBS macro from autotools doesn't
# appear to have an equivalent in CMake.
# If we need to link against nsl or socket then it
# needs to be specified manuall on the CMake command line.
# AC_SEARCH_LIBS(gethostbyname, nsl)
# AC_SEARCH_LIBS(socket, socket)

# websocketpp
# Debian: libwebsocketpp-dev
if(USE_WEBSOCKETPP)
   if(USE_NUGET)
      nuget_inst(WEBSOCKETPP "websocketpp-mtk.repack" "0.7.0-mtk19")
      message("Using ${WEBSOCKETPP_ROOT_DIR} and ${WEBSOCKETPP_LIBRARIES}")
      set(WEBSOCKETPP_LIBRARIES
            "${WEBSOCKETPP_LIBRARIES_DIR}/libwebsocketpp.lib")
      # FIXME - check if those libraries and headers really exist
      set(WEBSOCKETPP_FOUND TRUE)
   else()
      find_package(websocketpp REQUIRED)
   endif()
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

# HAVE_LIBDL from autotools obsolete,
# now we use CMAKE_DL_LIBS to include the library
# when necessary

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

if(BUILD_RECON)
   add_subdirectory(media)
endif()

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

# Create spec file for RPM packaging
# The tarball containing a spec file can be fed directly
# to the rpmbuild command.
configure_file(
   resiprocate.spec.in
   resiprocate.spec
   @ONLY)

# Add 'make dist' command for creating release tarball
set (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set (CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")

# pax appears to be the default, we need it due to some filenames
#set (COMPRESSION_OPTIONS --format=pax)

list(APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.git/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.gitignore")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeFiles/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/_CPack_Packages/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.deps/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.libs/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/.*\\\\.gz")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/.*\\\\.zip")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".*\\\\.o")
list(APPEND CPACK_SOURCE_IGNORE_FILES "lib.*\\\\.so*")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeCache.txt")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/contrib/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/debian/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "Makefile")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/config.h$")

include (CPack)
add_custom_target (dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

###############
### Summary ###
###############

include(FeatureSummary)
feature_summary(WHAT ALL)
