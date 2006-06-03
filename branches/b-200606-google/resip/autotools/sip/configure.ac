dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

AC_INIT([resiprocate/SipStack.hxx],, [resiprocate-devel@list.sipfoundry.org])

dnl
dnl Release Version
dnl
RESIP_MAJOR_VERSION=0
RESIP_MINOR_VERSION=9
RESIP_MICRO_VERSION=0
if test -e .svn; then
  RESIP_REVISION=`svn info | grep Revision | awk '{print $2}'`
else
  RESIP_REVISION='nosvn'
fi

RESIP_VERSION=$RESIP_MAJOR_VERSION.$RESIP_MINOR_VERSION.$RESIP_MICRO_VERSION-$RESIP_REVISION

#
#LIBRESIP_SO_VERSION=`expr $RESIP_MINOR_VERSION + $RESIP_MAJOR_VERSION`:$RESIP_MICRO_VERSION:$RESIP_MINOR_VERSION
# This SO version IS NOT related to the release (0.5.0) versions AT ALL.
# This needs to increment independently when the ABI to the library changes.
#
LIBRESIP_SO_VERSION=0:5:0

RESIP_PACKAGE=resiprocate

AC_SUBST(RESIP_VERSION)
AC_SUBST(RESIP_MAJOR_VERSION)
AC_SUBST(RESIP_MINOR_VERSION)
AC_SUBST(RESIP_MICRO_VERSION)
AC_SUBST(LIBRESIP_SO_VERSION, $LIBRESIP_SO_VERSION)

AC_MSG_RESULT([Configuring ${RESIP_PACKAGE} ${RESIP_VERSION}])


AC_CONFIG_SRCDIR([resiprocate/config.hxx.in])
AC_CONFIG_HEADER([resiprocate/config.hxx])
AM_INIT_AUTOMAKE($RESIP_PACKAGE, $RESIP_VERSION)

dnl AC_DISABLE_STATIC

AC_REVISION($Revision:$)

AM_MAINTAINER_MODE

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
AC_PROG_MAKE_SET
dnl AC_PROG_RANLIB

AC_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/sockio.h netinet/in.h sys/socket.h ])



AC_MSG_CHECKING([for libpopt])

popt_incdirs="/usr/include /opt/include /usr/pkg/include /usr/local/include /sw/include"
popt_incdir=NONE
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



AC_CHECK_STRUCT_FOR([
#include <sys/stat.h>
        ],
         stat,
         st_atime,
        [HAVE_ST_ATIME], )


dnl This is ugly, because I'm not an autoconf guru, but it works.
dnl Check for the type of socklen_t.
dnl Still thinking about how to incorporate the results into the code.

AC_CHECK_TYPE(socklen_t,
         AC_DEFINE([HAVE_SOCKLEN_T],[1],[ Has socklen_t ]), 
        [],
        [
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
]
        )

dnl Check for BSD socklen_t if we couldn't do it before.

if test $ac_cv_type_socklen_t = no; then
        unset ac_cv_type_socklen_t
        AC_CHECK_TYPE(socklen_t,
        AC_DEFINE([_BSD_SOCKLEN_T_],[int], [Needs BSD socklen_t]),
        [],
        [
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
# define _BSD_SOCKLEN_T_ int
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif        
        ]
        )
fi

RESIP_SSL_ENABLE
RESIP_LIB_ARES
RESIP_PROG_DISTCC
RESIP_IPV6
RESIP_GOOGLE_MALLOC
RESIP_GOOGLE_CPUPERF
RESIP_BOOST
RESIP_CURL
RESIP_SCANNER
RESIP_SCANNER_DEBUG
RESIP_EXCEPTION_DEBUG_LOGS
RESIP_SELECT_CONTENTS
RESIP_DATA_LOCAL_SIZE_CHECK

AC_DEFINE_UNQUOTED([RESIP_VERSION], 
        ["$RESIP_VERSION"],
         [The version number of resip])

AC_DEFINE_UNQUOTED([RESIP_MAJOR_VERSION],
         [$RESIP_MAJOR_VERSION],
         [The major version number of resip])

AC_DEFINE_UNQUOTED([RESIP_MINOR_VERSION],
         [$RESIP_MINOR_VERSION], 
        [The minor version number of resip])

AC_DEFINE_UNQUOTED([RESIP_MICRO_VERSION],
         [$RESIP_MICRO_VERSION], 
        [The micro version number of resip])


dnl Test for pthreads.

ACX_PTHREAD

AC_MSG_RESULT([Pthreads flags are : $PTHREAD_LIBS])

dnl LIBS="$PTHREAD_LIBS $LIBS"
dnl CFLAGS="-Wall -Wnodeprecated $CFLAGS $PTHREAD_CFLAGS"

dnl CC="$PTHREAD_CC"

dnl Temporary hack for kerberos on Red Hat 9 Linux
CPPFLAGS="$CPPFLAGS -I/usr/kerberos/include"
                                                                                
dnl Temporary hack until proper, portable pthreads detection is added.
LIBS="$LIBS -lpthread"



AC_CONFIG_FILES([Makefile resiprocate/Makefile resiprocate/test/Makefile resiprocate/dum/Makefile resiprocate/dum/test/Makefile])  

AC_OUTPUT
