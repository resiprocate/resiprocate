dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

AC_INIT([resiprocate/SipStack.hxx],, [resiprocate-devel@lists.sourceforge.net])

dnl
dnl Release Version
dnl
RESIP_MAJOR_VERSION=0
RESIP_MINOR_VERSION=3
RESIP_MICRO_VERSION=0

RESIP_VERSION=$RESIP_MAJOR_VERSION.$RESIP_MINOR_VERSION.$RESIP_MICRO_VERSION
LIBRESIP_SO_VERSION=`expr $RESIP_MINOR_VERSION + $RESIP_MAJOR_VERSION`:$RESIP_MICRO_VERSION:$RESIP_MINOR_VERSION

VERSION=$RESIP_VERSION
PACKAGE=resiprocate

AC_SUBST(RESIP_VERSION)
AC_SUBST(RESIP_MAJOR_VERSION)
AC_SUBST(RESIP_MINOR_VERSION)
AC_SUBST(RESIP_MICRO_VERSION)
AC_SUBST(LIBRESIP_SO_VERSION, $LIBRESIP_SO_VERSION)

AC_MSG_RESULT([Configuring ${PACKAGE} ${VERSION}])

dnl [resiprocate], $RESIP_VERSION, )

AC_CONFIG_SRCDIR([resiprocate/config.hxx.in])
AC_CONFIG_HEADER([resiprocate/config.hxx])
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_DISABLE_STATIC

AC_REVISION($Id: configure.ac,v 1.11 2003/09/14 08:43:57 alan Exp $)


AM_MAINTAINER_MODE

AC_LANG(C++)

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/sockio.h sys/int_types.h netinet/in.h])


AC_CHECK_STRUCT_FOR([#include <sys/stat.h>], stat, st_atime, HAVE_ST_ATIME, )


RESIP_LIB_OPENSSL
RESIP_LIB_ARES
RESIP_PROG_DISTCC
RESIP_IPV6
RESIP_SCANNER
RESIP_SCANNER_DEBUG

AC_DEFINE_UNQUOTED([RESIP_VERSION], ["$RESIP_VERSION"], [The version number of resip])
AC_DEFINE_UNQUOTED([RESIP_MAJOR_VERSION], [$RESIP_MAJOR_VERSION], [The major version number of resip])
AC_DEFINE_UNQUOTED([RESIP_MINOR_VERSION], [$RESIP_MINOR_VERSION], [The minor version number of resip])
AC_DEFINE_UNQUOTED([RESIP_MICRO_VERSION], [$RESIP_MICRO_VERSION], [The micro version number of resip])


dnl Test for pthreads.

ACX_PTHREAD

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="-Wall -Wnodeprecated $CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="-Wall -Wno-deprecated $CXXFLAGS"
CC="$PTHREAD_CC"

#dnl Temporary hack until proper, portable pthreads detection is added.
#LIBS="$LIBS -lpthread"

AC_CONFIG_FILES([Makefile resiprocate/Makefile resiprocate/test/Makefile])

AC_OUTPUT
