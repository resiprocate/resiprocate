
AC_INIT(resiprocate,1.8.0)
AC_CONFIG_SRCDIR(repro/repro.cxx)

ABIVERSION=1.8

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

AC_MSG_CHECKING([ for GNU warning flags])
if test "${GXX}"x = yesx; then
  CXXFLAGS="${CXXFLAGS} -Wall -Wno-deprecated"
  CFLAGS="${CFLAGS} -Wall -Wno-deprecated"
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

OSVER=`uname -r | tr '()/' " " | cut -f1 -d-`.0.0.0
OS_MAJOR_VER=`echo $OSVER | cut -f1 -d.`
OS_MINOR_VER=`echo $OSVER | cut -f2 -d.`
OS_POINT_VER=`echo $OSVER | cut -f3 -d.`
OS_PATCH_VER=`echo $OSVER | cut -f4 -d.`
AC_DEFINE_UNQUOTED(OS_MAJOR_VER, $OS_MAJOR_VER)
AH_TEMPLATE([OS_MAJOR_VER], [Major OS version])
AC_DEFINE_UNQUOTED(OS_MINOR_VER, $OS_MINOR_VER)
AH_TEMPLATE([OS_MINOR_VER], [Minor OS version])
AC_DEFINE_UNQUOTED(OS_POINT_VER, $OS_POINT_VER)
AH_TEMPLATE([OS_POINT_VER], [Point OS version])
AC_DEFINE_UNQUOTED(OS_PATCH_VER, $OS_PATCH_VER)
AH_TEMPLATE([OS_PATCH_VER], [Patch OS version])

case $host_cpu in
x86_64)
  LARCH=IA64
  ;;
i?86)
  LARCH=IA32
  ;;
*)
  echo "Unsupported host CPU: $host_cpu"
  echo "Please review the CPU-specific settings in configure.ac"
  echo "and then re-run autoreconf --install"
  exit 1
esac
ARCH=`echo $host_cpu | sed 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
ARCH_CFLAGS="-DRESIP_OSTYPE_LINUX -DRESIP_ARCH_${ARCH} -DRESIP_LARCH_${LARCH} -D_REENTRANT"

TOOLCHAIN_CFLAGS="-DRESIP_TOOLCHAIN_GNU"

CXXFLAGS="${CXXFLAGS} ${ARCH_CFLAGS} ${TOOLCHAIN_CFLAGS}"
CFLAGS="${CFLAGS} ${ARCH_CFLAGS} ${TOOLCHAIN_CFLAGS}"

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
 AC_DEFINE_UNQUOTED(USE_IPV6, , USE_IPV6), )

AC_ARG_ENABLE(dtls,
[  --enable-dtls           Enable DTLS support (requires OpenSSL)],
 [AC_DEFINE_UNQUOTED(USE_DTLS, 1, USE_DTLS)],  )
 
AM_CONDITIONAL(USE_MYSQL, false)
AC_ARG_WITH(mysql,
[  --with-mysql            Link against MySQL client libraries],
 [AC_DEFINE_UNQUOTED(USE_MYSQL, , USE_MYSQL)
 AC_SUBST(LIBMYSQL_LIBADD, "-lmysqlclient")
 AM_CONDITIONAL(USE_MYSQL, true)], 
 [ AC_SUBST(LIBMYSQL_LIBADD, "")])

AM_CONDITIONAL(USE_RADIUS_CLIENT, false)
AC_ARG_WITH(radius,
[  --with-radius           Link against RADIUS client libraries],
 [AC_DEFINE_UNQUOTED(USE_RADIUS_CLIENT, , USE_RADIUS_CLIENT)
 AC_SUBST(LIBRADIUS_LIBADD, "-lradiusclient-ng")
 AM_CONDITIONAL(USE_RADIUS_CLIENT, true)],
 [ AC_SUBST(LIBRADIUS_LIBADD, "")])

AM_CONDITIONAL(BUILD_TFM, false)
AC_ARG_WITH(tfm,
[  --with-tfm              Build TFM, links against Netxx and cppunit],
 [AC_DEFINE_UNQUOTED(BUILD_TFM, , BUILD_TFM)
 AM_CONDITIONAL(BUILD_TFM, true)], )

AM_CONDITIONAL(BUILD_APPS, false)
AC_ARG_WITH(apps,
[  --with-apps             Build apps, links against various things],
 [AC_DEFINE_UNQUOTED(BUILD_APPS, , BUILD_APPS)
 AM_CONDITIONAL(BUILD_APPS, true)], )

AM_CONDITIONAL(BUILD_RECON, false)
AC_ARG_WITH(recon,
[  --with-recon            Build recon, links against sipX],
 [AC_DEFINE_UNQUOTED(BUILD_RECON, , BUILD_RECON)
 AM_CONDITIONAL(BUILD_RECON, true)], )

AM_CONDITIONAL(BUILD_P2P, false)
AC_ARG_WITH(p2p,
[  --with-p2p              Build P2P, links against S2C and SSL, unfinished],
 [AC_DEFINE_UNQUOTED(BUILD_P2P, , BUILD_P2P)
 AM_CONDITIONAL(BUILD_P2P, true)], )

AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)

AM_MAINTAINER_MODE

AC_SUBST(ABIVERSION)

# this is temporarily removed:
#  apps/ichat-gw/jabberconnector/Makefile
# until it is tested with autotools
AC_OUTPUT(Makefile \
	rutil/Makefile \
	rutil/test/Makefile \
	resip/Makefile \
	resip/stack/Makefile \
	resip/stack/test/Makefile \
	resip/dum/Makefile \
	resip/dum/test/Makefile \
	resip/certs/Makefile \
	repro/Makefile \
	repro/reprocmd/Makefile \
	repro/test/Makefile \
	tfm/Makefile \
	tfm/tfdum/Makefile \
	apps/Makefile \
	apps/clicktocall/Makefile \
	apps/ichat-gw/Makefile \
	apps/sipdial/Makefile \
	resip.spec \
	reTurn/Makefile \
	reTurn/test/Makefile \
	reTurn/client/Makefile \
	reflow/Makefile \
	resip/recon/Makefile \
	presSvr/Makefile \
	p2p/s2c/s2c/Makefile \
	p2p/Makefile \
	)


