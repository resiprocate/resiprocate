dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

AC_INIT([resiprocate], [0.3], [resiprocate-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([1.7.5 nostdinc foreign])

AC_REVISION($Id: configure.ac,v 1.4 2003/06/17 04:16:10 rjsparks Exp $)

AM_MAINTAINER_MODE

AC_LANG(C++)

AC_CONFIG_SRCDIR([resiprocate/config.hxx.in])
AC_CONFIG_HEADER([resiprocate/config.hxx])  

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/sockio.h sys/int_types.h])

RESIP_LIB_OPENSSL
RESIP_LIB_ARES
RESIP_PROG_DISTCC

AC_CONFIG_FILES([Makefile resiprocate/Makefile])
AC_OUTPUT
