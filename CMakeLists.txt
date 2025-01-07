cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules")

# For Visual Studio Solution Generation
 set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release)
 set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
 set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

if(POLICY CMP0077)
   CMAKE_POLICY(SET CMP0077 NEW)
endif(POLICY CMP0077)

# https://cmake.org/cmake/help/latest/variable/PROJECT_VERSION.html
project(resiprocate VERSION 1.13.0)

# Specifies the pre-release tag, such as alphaN or betaN.
set(RESIPROCATE_VERSION_PRE_RELEASE "")

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
include(CheckCXXSourceCompiles)
include(CheckStructHasMember)

# Library version file generation.
add_definitions(-DHAVE_VERSION_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/resip/)

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
set(RESIP_SIP_MSG_MAX_BYTES 10485760)

# https://cmake.org/cmake/help/latest/module/TestBigEndian.html
# see also
# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_BYTE_ORDER.html
include (TestBigEndian)
test_big_endian(RESIP_BIG_ENDIAN)

check_cxx_source_compiles("
#include <string_view>
int main() {
    std::string_view sv = \"test\";
    return 0;
}
" RESIP_HAVE_STRING_VIEW)

CHECK_STRUCT_HAS_MEMBER(sockaddr_in sin_len arpa/inet.h HAVE_sockaddr_in_len)
if(HAVE_sockaddr_in_len)
   add_definitions(-DHAVE_sockaddr_in_len)
endif()

# Top-level user-settable variables (with defaults)
# Those can be queried from the command line using "cmake -LH" and can be
# specified on the command line, using cmake-gui or ccmake.
option(WITH_SSL "Link against SSL libraries" TRUE)
option(USE_POPT "Link against POPT libraries" TRUE)
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
option(USE_MAXMIND_GEOIP "Link against MaxMind GeoIP libraries" TRUE)
option(RESIP_HAVE_RADCLI "Link against radcli RADIUS client library" FALSE)
option(USE_NETSNMP "Link against NetSNMP client libraries" FALSE)
option(BUILD_REPRO "Build repro SIP proxy" TRUE)
option(BUILD_RETURN "Build reTurn server" TRUE)
option(BUILD_REFLOW "Build reflow library" TRUE)
option(BUILD_REND "Build rend" TRUE)
option(BUILD_TFM "Build TFM, requires Netxx and cppunit" TRUE)
option(BUILD_CLICKTOCALL "Build Click to call application" FALSE)
option(BUILD_ICHAT_GW "Build iChat gateway, requires gloox" FALSE)
option(BUILD_TELEPATHY_CM "Build Telepathy connection manager" FALSE)
option(BUILD_RECON "Build reCon Conversation Manager library" TRUE)
option(USE_SRTP1 "Use srtp 1.x instead of current version" FALSE)
option(BUILD_RECONSERVER "Build reConServer" FALSE)
option(USE_SIPXTAPI "Link against sipXtapi" FALSE)
option(USE_KURENTO "Build Kurento client (requires websocketpp)" FALSE)
option(USE_GSTREAMER "Link against Gstreamer" FALSE)
option(USE_LIBWEBRTC "Link against LibWebRTC" FALSE)
option(RECON_LOCAL_HW_TESTS "Attempt to use local audio hardware in unit tests" FALSE)
option(BUILD_P2P "Build P2P, links against S2C and SSL, unfinished" FALSE)
option(BUILD_PYTHON "Build components requiring Python" FALSE)
option(BUILD_TESTING "Build manual & unit tests" TRUE)

# Enable/disable linux based settings appropriately
if(WIN32)
   set(WITH_C_ARES_DEFAULT OFF)
   set(BUILD_QPID_PROTON_DEFAULT OFF)
   set(RESIP_ASSERT_SYSLOG_DEFAULT OFF)
   set(REGENERATE_MEDIA_SAMPLES_DEFAULT OFF)
   set(BUILD_DSO_PLUGINS_DEFAULT OFF)
else()
   set(WITH_C_ARES_DEFAULT ON)  # Needed for TFM to build correctly on linux
   set(BUILD_QPID_PROTON_DEFAULT ON)
   set(RESIP_ASSERT_SYSLOG_DEFAULT ON)
   set(REGENERATE_MEDIA_SAMPLES_DEFAULT ON)
   set(BUILD_DSO_PLUGINS_DEFAULT ON)
endif()
option(WITH_C_ARES "Link against libc-ares (rather than rutil/dns/ares)" ${WITH_C_ARES_DEFAULT})
option(BUILD_QPID_PROTON "Build components requiring qpid-proton (AMQP)" ${BUILD_QPID_PROTON_DEFAULT})
option(RESIP_ASSERT_SYSLOG "Log assertion failures with Syslog" ${RESIP_ASSERT_SYSLOG_DEFAULT})
option(REGENERATE_MEDIA_SAMPLES "Regenerate the header files containing raw audio samples (requires sox, xxd)" ${REGENERATE_MEDIA_SAMPLES_DEFAULT})
option(BUILD_DSO_PLUGINS "Build DSO plugins" ${BUILD_DSO_PLUGINS_DEFAULT})

if(WIN32)
    # Windows vcxproj files specify 10 in sipXtapi
    set(DEFAULT_BRIDGE_MAX_IN_OUTPUTS 10 CACHE STRING "recon: Maximum connections on bridge (must match value sipXtapi was compiled with)")
else()
    # Pre-build linux packages uses 20
    set(DEFAULT_BRIDGE_MAX_IN_OUTPUTS 20 CACHE STRING "recon: Maximum connections on bridge (must match value sipXtapi was compiled with)")
endif()

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

if(BUILD_TESTING)
    enable_testing()
endif()

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
   if(VERSIONED_SONAME AND BUILD_SHARED_LIBS)
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

set(USE_WEBSOCKETPP FALSE)

if(BUILD_REPRO)
   set(USE_BDB TRUE)
   set(USE_CAJUN TRUE)
endif()

if(BUILD_RECON)
   set(USE_SRTP TRUE)
   set(BUILD_RELOW TRUE)
   set(BUILD_RETURN TRUE)
endif()

if(BUILD_REFLOW)
   set(USE_SRTP TRUE)
   set(BUILD_RETURN TRUE)
endif()

if(BUILD_RETURN)
   set(USE_ASIO TRUE)
endif()

if(BUILD_TFM)
   set(USE_NETXX TRUE)
   set(USE_CPPUNIT TRUE)
endif()

if(BUILD_REND)
   set(USE_BOOST TRUE)
endif()

if(USE_KURENTO)
   set(USE_ASIO TRUE)
   set(USE_WEBSOCKETPP TRUE)
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
   pkg_check_modules(cares libcares REQUIRED IMPORTED_TARGET)

   set(USE_CARES true)
   add_definitions(-DUSE_CARES)

   set(ARES_LIBRARIES PkgConfig::cares)
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
   execute_process(
      COMMAND ${NUGET_EXE} install "${ARGV1}" -Version ${ARGV2} -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages
      COMMAND_ERROR_IS_FATAL ANY
      )
   set(${ARGV0}_PKG_DIR "${CMAKE_BINARY_DIR}/packages/${ARGV1}" PARENT_SCOPE)
endfunction()

# OpenSSL
if(WITH_SSL)
   if(USE_NUGET)
      nuget_inst(OPENSSL "zeroc.openssl.v142" "1.1.1.3")  # VS2019 compatible version, also compatible with VS2022
      message("Using ${OPENSSL_ROOT_DIR} and ${OPENSSL_LIBRARIES_DIR}")
      # this is an argument to FindOpenSSL
      # https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
      set(OPENSSL_ROOT_DIR ${OPENSSL_PKG_DIR}/build/native)
      set(OPENSSL_USE_STATIC_LIBS True)
      set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
      set(OPENSSL_LIBRARIES OPENSSL OPENSSL_CRYPTO)
      set(OPENSSL_FOUND TRUE)

      add_library(OPENSSL SHARED IMPORTED GLOBAL)
      add_library(OPENSSL_CRYPTO SHARED IMPORTED GLOBAL)

      set_target_properties(OPENSSL_CRYPTO PROPERTIES
         IMPORTED_IMPLIB_RELEASE "${OPENSSL_ROOT_DIR}/lib/${WIN_ARCH}/Release/libcrypto.lib"
         IMPORTED_IMPLIB_DEBUG "${OPENSSL_ROOT_DIR}/lib/${WIN_ARCH}/Debug/libcrypto.lib"
         IMPORTED_LOCATION_RELEASE "${OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Release/libcrypto-1_1-${WIN_ARCH}.dll"
         IMPORTED_LOCATION_DEBUG "${OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Debug/libcrypto-1_1-${WIN_ARCH}.dll"
         INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_ROOT_DIR}/include"
      )

      # Note: We are adding the crypto library to the OPENSSL IMPORTED_LOCATION so that custom PostBuild copy steps only need to specify OPENSSL targets
      set_target_properties(OPENSSL PROPERTIES
         IMPORTED_IMPLIB_RELEASE "${OPENSSL_ROOT_DIR}/lib/${WIN_ARCH}/Release/libssl.lib"
         IMPORTED_IMPLIB_DEBUG "${OPENSSL_ROOT_DIR}/lib/${WIN_ARCH}/Debug/libssl.lib"
         IMPORTED_LOCATION_RELEASE "${OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Release/libssl-1_1-${WIN_ARCH}.dll;{OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Release/libcrypto-1_1-${WIN_ARCH}.dll"
         IMPORTED_LOCATION_DEBUG "${OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Debug/libssl-1_1-${WIN_ARCH}.dll;${OPENSSL_ROOT_DIR}/bin/${WIN_ARCH}/Debug/libcrypto-1_1-${WIN_ARCH}.dll"
         IMPORTED_LINK_INTERFACE_LIBRARIES "OPENSSL_CRYPTO"
         INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_ROOT_DIR}/include"
      )

      install(IMPORTED_RUNTIME_ARTIFACTS OPENSSL_CRYPTO OPENSSL RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
      install(IMPORTED_RUNTIME_ARTIFACTS OPENSSL_CRYPTO OPENSSL RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
      install(IMPORTED_RUNTIME_ARTIFACTS OPENSSL_CRYPTO OPENSSL RUNTIME DESTINATION tfm)
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
      set(MySQL_LIBRARIES "${MySQL_ROOT}/lib/Release/libmysql.lib") # TODO Fix this
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
      # message(FATAL_ERROR "GeoIP has been deprecated upstream, see https://github.com/maxmind/geoip-api-c")
      add_subdirectory(contrib/GeoIP)
      set(GEOIP_LIBRARIES libGeoIP)
      set(GEOIP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/GeoIP/libGeoIP")
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
      set(NETXX_LIBRARIES Netxx)
   else()
      find_package(Netxx REQUIRED)
   endif()
endif()

# cppunit
# Debian: libcppunit-dev
if(USE_CPPUNIT)
   if(USE_CONTRIB)
      add_subdirectory(contrib/cppunit/src/cppunit)
      set(CPPUNIT_LIBRARIES cppunit)
   else()
      pkg_check_modules(CPPUNIT cppunit REQUIRED)
   endif()
endif()

# BerkeleyDb
# Debian: libdb++-dev
if(USE_BDB)
   if(USE_NUGET)
      # https://www.nuget.org/packages/berkeley.db.v140
      nuget_inst(BERKELEYDB "berkeley.db.v140" "5.3.28.3")
      set(BERKELEYDB_ROOT_DIR "${BERKELEYDB_PKG_DIR}/build/native")
      set(BERKELEYDB_INCLUDE_DIRS "${BERKELEYDB_ROOT_DIR}/include")
      set(BERKELEYDB_FOUND TRUE)

      add_library(BERKELEYDB SHARED IMPORTED GLOBAL)
      set_target_properties(BERKELEYDB PROPERTIES
         IMPORTED_IMPLIB_RELEASE "${BERKELEYDB_ROOT_DIR}/lib/${WIN_ARCH}/Release/libdb53.lib"
         IMPORTED_IMPLIB_DEBUG "${BERKELEYDB_ROOT_DIR}/lib/${WIN_ARCH}/Debug/libdb53d.lib"
         IMPORTED_LOCATION_RELEASE "${BERKELEYDB_ROOT_DIR}/bin/${WIN_ARCH}/Release/libdb53.dll"
         IMPORTED_LOCATION_DEBUG "${BERKELEYDB_ROOT_DIR}/bin/${WIN_ARCH}/Debug/libdb53d.dll"
         INTERFACE_INCLUDE_DIRECTORIES "${BERKELEYDB_ROOT_DIR}/include"
      )

      install(IMPORTED_RUNTIME_ARTIFACTS BERKELEYDB RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
      install(IMPORTED_RUNTIME_ARTIFACTS BERKELEYDB RUNTIME DESTINATION tfm)
   elseif(USE_CONTRIB)
      add_subdirectory(contrib/db)
   else()
      find_package(BerkeleyDb REQUIRED)
   endif()
   set(DB_HEADER "db_cxx.h")
endif()

# Cajun
# Debian: libcajun-dev
if(USE_CAJUN)
   #if(USE_CONTRIB)
      # FIXME: convert Cajun to CMake build system
      # FIXME: include Cajun as a Git submodule or NuGet
      #add_subdirectory(contrib/cajun)
      set(CAJUN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cajun/include")
   #else()
   #   find_package(cajun REQUIRED)
   #endif()
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
      message("Using ${BOOST_PKG_DIR}")
      set(BOOST_INCLUDE_DIRS "${BOOST_PKG_DIR}/lib/native/include")
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
      if(USE_CONTRIB)
         add_subdirectory("contrib/srtp")

         set(SRTP2_INCLUDE_DIRS "${libsrtp2_SOURCE_DIR}/include")
         set_property(TARGET srtp2 PROPERTY IMPORTED_IMPLIB_RELEASE "${libsrtp2_BINARY_DIR}/Release/srtp2.lib")
         set_property(TARGET srtp2 PROPERTY IMPORTED_IMPLIB_DEBUG "${libsrtp2_BINARY_DIR}/Debug/srtp2.lib")

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
   if(WIN32)
      # Include VS2022 projects for sipXtapi - TODO Detect VS version and use other versions appropriately
      # Note:  this requires the user the manually pull sipXtapi source code into a folder that is called sipXtapi and
      #        is at the same folder level at the resiprocate git repro, ie:
      #        <basedir>/resiprocate/<resip-git-root>
      #        <basedir>/sipXtapi/<sipXtapi-git-root>
      set(SIPXTAPI_ROOT_DIR "${CMAKE_SOURCE_DIR}/../sipXtapi")
      message("Using SIPXTAPI_ROOT_DIR=${SIPXTAPI_ROOT_DIR}")
      include_external_msproject(sipXtapi-pcre ${SIPXTAPI_ROOT_DIR}/sipXportLib/contrib/pcre/pcre_17_0.vcxproj)
      include_external_msproject(sipXportLib ${SIPXTAPI_ROOT_DIR}/sipXportLib/sipXportLib-msvc17.vcxproj)
      include_external_msproject(sipXsdpLib ${SIPXTAPI_ROOT_DIR}/sipXsdpLib/sipXsdpLib-msvc17.vcxproj)
      include_external_msproject(sipXmediaLib ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/sipXmediaLib-msvc17.vcxproj)
      include_external_msproject(sipXmediaAdapterLib ${SIPXTAPI_ROOT_DIR}/sipXmediaAdapterLib/sipXmediaAdapterLib-msvc17.vcxproj)

      # Setup VS folders
      set_target_properties(sipXtapi-pcre PROPERTIES FOLDER sipXtapi)
      set_target_properties(sipXportLib PROPERTIES FOLDER sipXtapi)
      set_target_properties(sipXsdpLib PROPERTIES FOLDER sipXtapi)
      set_target_properties(sipXmediaLib PROPERTIES FOLDER sipXtapi)
      set_target_properties(sipXmediaAdapterLib PROPERTIES FOLDER sipXtapi)

      # Setup Includes
      set(SIPXTAPI_INCLUDE_DIRS 
          "${SIPXTAPI_ROOT_DIR}/sipXportLib/include"
          "${SIPXTAPI_ROOT_DIR}/sipXsdpLib/include"
          "${SIPXTAPI_ROOT_DIR}/sipXmediaLib/include"
          "${SIPXTAPI_ROOT_DIR}/sipXtackLib/include"
          "${SIPXTAPI_ROOT_DIR}/sipXcallLib/include"
          "${SIPXTAPI_ROOT_DIR}/sipXmediaAdapterLib/interface"
          "${SIPXTAPI_ROOT_DIR}/sipXmediaAdapterLib/sipXmediaMediaProcessing/include"
      )

      # No need to setup libraries, executables will use add_dependencies to get VS linker to link in appropriate libs

      # Codec contrib projects
      include_external_msproject(libg729a ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libg729a/libg729a-msvc17.vcxproj)
      include_external_msproject(libgsm ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libgsm/libgsm-msvc17.vcxproj)
      include_external_msproject(libilbc ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libilbc/VS2022/libilbc-msvc17.vcxproj)
      include_external_msproject(libogg ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libopus/libogg/win32/VS2022/libogg.vcxproj)
      include_external_msproject(libspeex ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libspeex/win32/VS2022/libspeex/libspeex.vcxproj)
      include_external_msproject(libspeexdsp ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libspeex/win32/VS2022/libspeexdsp/libspeexdsp.vcxproj)
      include_external_msproject(opus ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libopus/opus/win32/VS2022/opus.vcxproj)
      include_external_msproject(opusenc ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libopus/libopusenc/win32/VS2022/opusenc.vcxproj)
      include_external_msproject(opusfile ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/contrib/libopus/opusfile/win32/VS2022/opusfile.vcxproj)
      # Make sure we pickup the NoHTTP versions of these projects
      set_property(TARGET opusfile PROPERTY MAP_IMPORTED_CONFIG_RELEASE "Release-NoHTTP")
      set_property(TARGET opusfile PROPERTY MAP_IMPORTED_CONFIG_DEBUG "Debug-NoHTTP")

      # Setup VS folders
      set_target_properties(libg729a PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(libgsm PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(libilbc PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(libogg PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(libspeex PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(libspeexdsp PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(opus PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(opusenc PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)
      set_target_properties(opusfile PROPERTIES FOLDER sipXtapi/ThirdPartyCodecs)


      # Codec plugin projects
      include_external_msproject(plgg729 ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgg729/plgg729-msvc17.vcxproj)
      include_external_msproject(plggsm ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plggsm/plggsm-msvc17.vcxproj)
      include_external_msproject(plgilbc ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgilbc/plgilbc-msvc17.vcxproj)
      include_external_msproject(plgl16 ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgl16/plgl16-msvc17.vcxproj)
      include_external_msproject(plgopus ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgopus/plgopus-msvc17.vcxproj)
      include_external_msproject(plgpcmapcmu ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgpcmapcmu/plgpcmapcmu-msvc17.vcxproj)
      include_external_msproject(plgspeex ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgspeex/plgspeex-msvc17.vcxproj)
      include_external_msproject(plgtones ${SIPXTAPI_ROOT_DIR}/sipXmediaLib/src/mp/codecs/plgtones/plgtones-msvc17.vcxproj)

      # Setup VS folders
      set_target_properties(plgg729 PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plggsm PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgilbc PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgl16 PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgopus PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgpcmapcmu PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgspeex PROPERTIES FOLDER sipXtapi/CodecPlugins)
      set_target_properties(plgtones PROPERTIES FOLDER sipXtapi/CodecPlugins)

   else()
      find_package(sipXtapi REQUIRED)
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
   pkg_check_modules(QPIDPROTON libqpid-proton-cpp REQUIRED)
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
      message("Using ${WEBSOCKETPP_PKG_DIR}")
      set(WEBSOCKETPP_INCLUDE_DIRS "${PKG_DIR}/build/native/include")
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
   add_compile_definitions(_WIN32_WINNT=0x0601)
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

if(BUILD_REFLOW)
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
