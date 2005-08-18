#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([repro], [0.1], [resiprocate-devel@lists.sipfoundry.org])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([repro.cxx])
AC_CONFIG_HEADER([config.hxx])

# Checks for programs.
AC_LANG(C++)
AC_PROG_CXX

AC_MSG_CHECKING([ for GNU warning flags])

if test "${GXX}"x = yesx; then
        CXXFLAGS="${CXXFLAGS} -Wall -Wno-deprecated"
        CFLAGS="${CFLAGS} -Wall -Wno-deprecated"
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

AC_PROG_INSTALL

AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])

AC_MSG_CHECKING([for libpopt])

popt_incdirs="/usr/include /opt/include /usr/pkg/include /usr/local/include /sw/include"
for dir in $popt_incdirs; do
  try="ls -1 $dir/popt.h"
  if test "`eval $try 2> /dev/null`"; then
     popt_incdir=$dir;
     break;
  else
     echo "tried $dir" >&AC_FD_CC ;
  fi
done

popt_libdirs="/usr/lib /opt/lib /usr/pkg/lib /usr/local/lib /sw/lib"
popt_libdir=NONE
for dir in $popt_libdirs; do
  try="ls -1 $dir/libpopt*"
  if test "`eval $try 2> /dev/null`"; then
     popt_libdir=$dir;
     break;
  else
        echo "tried $dir" >&AC_FD_CC ;
  fi
done

if test "$popt_libdir" = "NONE" || test "$popt_incdir" = "NONE"; then
   AC_MSG_RESULT([no])
else
   AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_POPT_H], [1], [ Has popt.h ])    
   LIBS="$LIBS -L$popt_libdir -lpopt"
   CPPFLAGS="$CPPFLAGS -I$popt_incdir"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([regcomp socket strerror])

RESIP_SSL_ENABLE
RESIP_IPV6
RESIP_PROG_DISTCC
RESIP_CHECK_MYSQL
RESIP_LIB_RESIP

if test "$MYSQL_INCLUDES" && test "$MYSQL_LIBS"; then
  CPPFLAGS="$CPPFLAGS $MYSQL_INCLUDES -DUSE_MYSQL"
  LIBS="$LIBS $MYSQL_LIBS"
fi

AX_BERKELEY_DB([4.0],
    [
	LIBS="$LIBS $DB_LIBS"
	],
	[
	AC_MSG_ERROR([missing berkeley DB])
	]
)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
