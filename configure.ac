
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

AH_TEMPLATE([HAVE_sockaddr_in_len], [Define if sockaddr_in.sin_len exists])
AC_CHECK_MEMBER([struct sockaddr_in.sin_len], AC_DEFINE(HAVE_sockaddr_in_len),,
 [#include <arpa/inet.h>])

AC_DEFINE(REPRO_BUILD_REV, PACKAGE_VERSION, REPRO_BUILD_REV)
AH_TEMPLATE([REPRO_BUILD_REV], [SVN revsion])
AC_DEFINE_UNQUOTED(REPRO_BUILD_HOST, "`hostname`", REPRO_BUILD_HOST)
AH_TEMPLATE([REPRO_BUILD_HOST], [Host where package was configured])
AC_DEFINE(REPRO_RELEASE_VERSION, PACKAGE_VERSION, REPRO_RELEASE_VERSION)
AH_TEMPLATE([REPRO_RELEASE_VERSION], [Package version])
AC_DEFINE_UNQUOTED(DB_HEADER, "db_cxx.h", DB_HEADER)
AH_TEMPLATE([DB_HEADER], [Name of header for libdb])

AM_CONDITIONAL(USE_SSL, false)
AC_ARG_WITH(ssl, 
[  --with-ssl              Link against OpenSSL libraries],
 [AC_DEFINE_UNQUOTED(USE_SSL, , USE_SSL)
 AC_SUBST(LIBSSL_LIBADD, "-lssl")
 AM_CONDITIONAL(USE_SSL, true)],
 [ AC_SUBST(LIBSSL_LIBADD, "")])

AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           Enable IPv6 support],
 AC_DEFINE_UNQUOTED(USE_IPV6, "yes", USE_IPV6), )

AC_ARG_ENABLE(dtls,
[  --enable-dtls           Enable DTLS support (requires OpenSSL)],
 [AC_DEFINE_UNQUOTED(USE_DTLS, 1, USE_DTLS)],  )
 

AC_ARG_WITH(mysql,
[  --with-mysql            Link against MySQL client libraries],
 AC_DEFINE_UNQUOTED(USE_MYSQL, "yes", USE_MYSQL), )

AC_ARG_WITH(radius,
[  --with-radius           Link against RADIUS client libraries],
 AC_DEFINE_UNQUOTED(USE_RADIUS_CLIENT, "yes", USE_RADIUS_CLIENT), )

AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)

AM_MAINTAINER_MODE

AC_OUTPUT(Makefile \
	rutil/Makefile \
	rutil/test/Makefile \
	resip/Makefile \
	resip/stack/Makefile \
	resip/stack/test/Makefile \
	resip/dum/Makefile \
	repro/Makefile \
	contrib/ares/Makefile \
	resip.spec \
	reTurn/Makefile \
	reTurn/client/Makefile \
	presSvr/Makefile \
	p2p/s2c/s2c/Makefile \
	p2p/Makefile \
	)


