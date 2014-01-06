cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# TODO wire these up.
option(BUILD_APPS "BUILD_APPS" OFF)
option(BUILD_ICHAT_GW "BUILD_ICHAT_GW" OFF)
option(BUILD_P2P "BUILD_P2P" OFF)
option(BUILD_PYTHON "BUILD_PYTHON" OFF)
option(BUILD_RECON "BUILD_RECON" OFF)
option(BUILD_REND "BUILD_REND" OFF)
option(BUILD_REPRO "BUILD_REPRO" OFF)
option(BUILD_TFM "BUILD_TFM" OFF)


option(HAVE_CLOCK_GETTIME_MONOTONIC "HAVE_CLOCK_GETTIME_MONOTONIC" OFF)
option(HAVE_DLFCN_H "HAVE_DLFCN_H" OFF)
option(HAVE_EPOLL "HAVE_EPOLL" OFF)
option(HAVE_POPT_H "HAVE_POPT_H" OFF)
option(PEDANTIC_STACK "PEDANTIC_STACK" OFF)
option(RESIP_BIG_ENDIAN "RESIP_BIG_ENDIAN" OFF)
option(RESIP_HAVE_FREERADIUS_CLIENT "RESIP_HAVE_FREERADIUS_CLIENT" OFF)
option(USE_ARES "USE_ARES" OFF)
option(USE_CARES "USE_CARES" OFF)
option(USE_DTLS "USE_DTLS" OFF)
option(USE_IPV6 "USE_IPV6" OFF)
option(USE_MAXMIND_GEOIP "USE_MAXMIND_GEOIP" OFF)
option(USE_MYSQL "USE_MYSQL" OFF)
option(USE_RADIUS_CLIENT "USE_RADIUS_CLIENT" OFF)
option(USE_SIGCOMP "USE_SIGCOMP" OFF)
option(USE_SSL "USE_SSL" OFF)

#cmakedefine RESIP_SIP_MSG_MAX_BYTES
#cmakedefine DB_HEADER
#cmakedefine OS_MAJOR_VER
#cmakedefine OS_MINOR_VER
#cmakedefine OS_PATCH_VER
#cmakedefine OS_POINT_VER
#cmakedefine PACKAGE
#cmakedefine PACKAGE_BUGREPORT
#cmakedefine PACKAGE_NAME
#cmakedefine PACKAGE_STRING
#cmakedefine PACKAGE_TARNAME
#cmakedefine PACKAGE_URL
#cmakedefine PACKAGE_VERSION
#cmakedefine REPRO_BUILD_HOST
#cmakedefine REPRO_BUILD_REV
#cmakedefine REPRO_DSO_PLUGINS
#cmakedefine REPRO_RELEASE_VERSION
#cmakedefine VERSION
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(rutil)
add_subdirectory(resip)

if (BUILD_REPRO)
  add_subdirectory(repro)
endif (BUILD_REPRO)


########### install files ###############




#original Makefile.am contents follow:

#
#
#AUTOMAKE_OPTIONS = foreign dist-tarZ
#ACLOCAL_AMFLAGS = -I m4
#
#SUBDIRS = rutil
#SUBDIRS += resip
#if BUILD_REPRO
#SUBDIRS += repro
#endif
#if BUILD_TFM
#SUBDIRS += tfm
#endif
#
#if BUILD_APPS
#SUBDIRS += apps
#endif
#
## The Makefiles for these don't separate SSL stuff yet...
#if USE_SSL
#SUBDIRS += reTurn
#if BUILD_RECON
#SUBDIRS += reflow
#SUBDIRS += resip/recon
#endif
#SUBDIRS += presSvr
#if BUILD_P2P
#SUBDIRS += p2p/s2c/s2c p2p
#endif
#endif
#
##EXTRA_DIST = build-contrib.sh
#
#EXTRA_DIST = reSIProcate_10_0.sln
##EXTRA_DIST += reSIProcate_7_1.sln
#EXTRA_DIST += reSIProcate_8_0.sln
#EXTRA_DIST += reSIProcate_9_0.sln
#
#EXTRA_DIST += resiprocate.spec
#
#EXTRA_DIST += RELEASE-PROCESS.txt
#EXTRA_DIST += build/configure-android.sh
#
