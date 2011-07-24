
AC_INIT(resiprocate,1.8)
AC_CONFIG_SRCDIR(repro/repro.cxx)

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX

AC_PROG_INSTALL

AC_PROG_LIBTOOL

AC_PROG_SED

AM_MISSING_PROG([GPERF], [gperf])

AC_OUTPUT(Makefile \
	rutil/Makefile \
	resip/Makefile \
	resip/stack/Makefile \
	resip/dum/Makefile \
	)


