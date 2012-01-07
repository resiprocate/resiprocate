
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

AC_PROG_MAKE_SET
AC_PROG_RANLIB

AM_MISSING_PROG([GPERF], [gperf])

AC_DEFINE(REPRO_BUILD_REV, PACKAGE_VERSION, REPRO_BUILD_REV)
AH_TEMPLATE([REPRO_BUILD_REV], [SVN revsion])
AC_DEFINE_UNQUOTED(REPRO_BUILD_HOST, "`hostname`", REPRO_BUILD_HOST)
AH_TEMPLATE([REPRO_BUILD_HOST], [Host where package was configured])
AC_DEFINE(REPRO_RELEASE_VERSION, PACKAGE_VERSION, REPRO_RELEASE_VERSION)
AH_TEMPLATE([REPRO_RELEASE_VERSION], [Package version])
AC_DEFINE_UNQUOTED(DB_HEADER, "db_cxx.h", DB_HEADER)
AH_TEMPLATE([DB_HEADER], [Name of header for libdb])

AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)

AM_MAINTAINER_MODE

AC_OUTPUT(Makefile \
	rutil/Makefile \
	rutil/test/Makefile \
	resip/Makefile \
	resip/stack/Makefile \
	resip/dum/Makefile \
	repro/Makefile \
	contrib/ares/Makefile \
	resip.spec \
	reTurn/Makefile \
	reTurn/client/Makefile \
	presSvr/Makefile \
	p2p/s2c/s2c/Makefile \
	)


