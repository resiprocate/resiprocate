# gc.m4 serial 4
dnl Copyright (C) 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GC],
[
  AC_ARG_WITH(libgcrypt,
    AS_HELP_STRING([--with-libgcrypt], [use libgcrypt for low-level crypto]),
    libgcrypt=$withval, libgcrypt=no)
  if test "$libgcrypt" != no; then
    AC_LIB_HAVE_LINKFLAGS([gcrypt], [], [#include <gcrypt.h>])
  fi
  if test "$ac_cv_libgcrypt" = yes; then
    AC_LIBOBJ([gc-libgcrypt])
  else
    AC_LIBOBJ([gc-gnulib])
  fi
])

# Prerequisites of lib/gc.h
AC_DEFUN([gl_PREREQ_GC],
[
  AC_REQUIRE([AC_C_RESTRICT])
  :
])
