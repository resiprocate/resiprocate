# gprefver.m4 serial 1

dnl Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007 Free
dnl Software Foundation, Inc.
dnl
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Determine the parameter type used by the available gperf

AC_DEFUN([AC_GPERFVER],

GPERFVER=`gperf -v|head -1 |cut -d " " -f3`
if test "$GPERFVER"x != x; then
  GPERF_MAJOR_VER=`echo $GPERFVER | cut -f1 -d.`
  GPERF_MINOR_VER=`echo $GPERFVER | cut -f2 -d.`
  if test  $GPERF_MAJOR_VER -gt 3 -o $GPERF_MAJOR_VER -eq 3 -a $GPERF_MINOR_VER -ge 1 ; then
    GPERF_SIZE_TYPE="size_t"
  else
    GPERF_SIZE_TYPE="unsigned int"
  fi
else
  GPERF_SIZE_TYPE="size_t"
fi

AC_DEFINE_UNQUOTED(GPERF_SIZE_TYPE, $GPERF_SIZE_TYPE)
AH_TEMPLATE([GPERF_SIZE_TYPE], [GPERF size type])
)


