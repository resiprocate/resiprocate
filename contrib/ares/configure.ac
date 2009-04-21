dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.51)

AC_INIT([ares_init.c],,)

PACKAGE=ares
VERSION=1.1.2
ARES_VERSION=${VERSION}

AC_MSG_RESULT([Configuring ${PACKAGE} ${VERSION}])

dnl AC_CONFIG_SRCDIR([config.h.in])
dnl AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE(${PACKAGE}, ${VERSION})
AC_SUBST(ARES_VERSION)

AC_REVISION($Revision:$)
AC_LANG(C)
dnl AC_PROG_GXX

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_CANONICAL_HOST
case $host_os in
solaris*)
	 AC_DEFINE([ETC_INET], [], [Solaris ETC_INET flag])
	;;
esac

AC_ARG_WITH(ipv6,
      AC_HELP_STRING([--with-ipv6],[enable ipv6]),
      [CFLAGS="${CFLAGS} -DUSE_IPV6=1"])

AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)

AM_MAINTAINER_MODE

AC_OUTPUT(Makefile)

