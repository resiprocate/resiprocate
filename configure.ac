
AC_INIT(resiprocate,1.13.0~alpha1)
AC_CONFIG_SRCDIR(repro/repro.cxx)

SO_RELEASE=`echo $PACKAGE_VERSION | cut -f1,2 -d.`
AC_SUBST(SO_RELEASE)


########## End of release/version data ######################

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax])
m4_pattern_forbid([^PKG_])

AC_CANONICAL_HOST
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX

AC_PROG_INSTALL

AC_PROG_LIBTOOL

AC_PROG_SED

AC_PROG_MAKE_SET

AM_MISSING_PROG([GPERF], [gperf])

AC_GPERFVER

AX_CXX_COMPILE_STDCXX(11)

CPPFLAGS="${CPPFLAGS} -DENABLE_LOG_REPOSITORY_DETAILS"

AC_MSG_CHECKING([ for GNU warning flags])
if test "${GXX}"x = yesx; then
  CXXFLAGS="${CXXFLAGS} -Wall -Wno-deprecated"
  CFLAGS="${CFLAGS} -Wall -Wno-deprecated"
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

ARCH_CPPFLAGS="-D_REENTRANT"

AM_CONDITIONAL([MACOSX], [false])
AS_IF([test "x$host_vendor" = "xapple"], [
  AC_MSG_CHECKING([if building for macOS])
  AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[
#include <stdio.h>
#include <TargetConditionals.h>
    ]], [[
#if TARGET_OS_IPHONE == 1
#error Not macOS
#endif
return 0;
    ]])
  ],[
    AC_MSG_RESULT([yes])
    AM_CONDITIONAL([MACOSX], [true])
  ],[
    AC_MSG_RESULT([no])
  ])
])

AC_C_BIGENDIAN([AC_DEFINE_UNQUOTED(RESIP_BIG_ENDIAN, , RESIP_BIG_ENDIAN)])

CPPFLAGS="${CPPFLAGS} ${ARCH_CPPFLAGS}"

AH_TEMPLATE([HAVE_sockaddr_in_len], [Define if sockaddr_in.sin_len exists])
AC_CHECK_MEMBER([struct sockaddr_in.sin_len], AC_DEFINE(HAVE_sockaddr_in_len),,
 [#include <arpa/inet.h>])

AC_DEFINE(REPRO_BUILD_REV, PACKAGE_VERSION, REPRO_BUILD_REV)
AH_TEMPLATE([REPRO_BUILD_REV], [SVN revision])
AC_DEFINE_UNQUOTED(REPRO_BUILD_HOST, "localhost", REPRO_BUILD_HOST)
AH_TEMPLATE([REPRO_BUILD_HOST], [Host where package was configured])
AC_DEFINE(REPRO_RELEASE_VERSION, PACKAGE_VERSION, REPRO_RELEASE_VERSION)
AH_TEMPLATE([REPRO_RELEASE_VERSION], [Package version])
AC_DEFINE_UNQUOTED(DB_HEADER, "db_cxx.h", DB_HEADER)
AH_TEMPLATE([DB_HEADER], [Name of header for libdb])

AC_DEFINE_UNQUOTED(RESIP_SIP_MSG_MAX_BYTES, ${RESIP_SIP_MSG_MAX_BYTES:-10485760}, [Maximum SIP message size to try and parse (bytes)])

AM_CONDITIONAL(USE_ARES, true)
AM_CONDITIONAL(USE_CARES, false)
AC_ARG_WITH(c-ares,
[  --with-c-ares           Link against libc-ares (rather than contrib/ares)],
 [AC_DEFINE_UNQUOTED(USE_CARES, , USE_CARES)
 AC_SUBST(LIBARES_LIBADD, "-lcares")
 AM_CONDITIONAL(USE_ARES, false)
 AM_CONDITIONAL(USE_CARES, true)
 AC_SEARCH_LIBS(ares_init, cares, , [
     AC_MSG_ERROR([unable to find ares_init(), install libcares and/or set LDFLAGS, or use contrib/ares])
   ])],
 [ AC_DEFINE_UNQUOTED(USE_ARES, , USE_ARES)
   CXXFLAGS="$CXXFLAGS -I\$(top_srcdir)/rutil/dns/ares"
   LDFLAGS="$LDFLAGS -L\$(top_builddir)/rutil/dns/ares"
   AC_SUBST(LIBARES_LIBADD, "\$(top_builddir)/rutil/dns/ares/libresipares.la")
   # We no longer check for this because it gets built by us:
   #AC_SEARCH_LIBS(ares_init, ares, , [
   #  AC_MSG_ERROR([unable to find ares_init(), must build contrib/ares or specify --with-c-ares])
   #])
   ])

AM_CONDITIONAL(USE_SSL, false)
AC_ARG_WITH(ssl, 
[  --with-ssl              Link against OpenSSL libraries],
 [AC_DEFINE_UNQUOTED(USE_SSL, , USE_SSL)
 AC_SUBST(LIBSSL_LIBADD, "-lssl -lcrypto")
 AM_CONDITIONAL(USE_SSL, true)],
 [ AC_SUBST(LIBSSL_LIBADD, "")])

AM_CONDITIONAL(USE_POPT, false)
AC_ARG_WITH(popt,
[  --with-popt             Link against POPT libraries],
 [AC_DEFINE_UNQUOTED(HAVE_POPT_H, , HAVE_POPT_H)
 AC_SUBST(LIBPOPT_LIBADD, "-lpopt")
 AM_CONDITIONAL(USE_POPT, true)],
 [ AC_SUBST(LIBPOPT_LIBADD, "")])

AC_ARG_WITH(sigcomp,
[  --with-sigcomp          Link against Open SigComp libraries for SigComp],
 AC_DEFINE_UNQUOTED(USE_SIGCOMP, , USE_SIGCOMP), )

AM_CONDITIONAL(USE_FMT, false)
AC_ARG_WITH(fmt,
[  --with-fmt              Link against fmt library],
 [AC_DEFINE_UNQUOTED(USE_FMT, , USE_FMT)
 PKG_CHECK_MODULES([DEPS_FMT], [fmt])
 AM_CONDITIONAL(USE_FMT, true)])

#LIBTOOL_VERSION_RELEASE="-version-info 0:0:0 -release @SO_RELEASE@"
LIBTOOL_VERSION_RELEASE="-release $SO_RELEASE"
AC_SUBST(LIBTOOL_VERSION_RELEASE)
AC_ARG_ENABLE(versioned-soname,
[  --enable-versioned-soname Enable version in SONAME],
   [ if test "$enableval" = no; then
       LIBTOOL_VERSION_RELEASE=""
       AC_SUBST(LIBTOOL_VERSION_RELEASE)
     fi
   ],
   [
   ])

AC_ARG_ENABLE(android,
[  --enable-android        Enable Android build],
   [AC_SUBST(LIBPTHREAD_LIBADD, "")
   AC_SUBST(LIBLOG_LIBADD, "-llog")],
   [AC_SUBST(LIBPTHREAD_LIBADD, "-lpthread")
   AC_SUBST(LIBLOG_LIBADD, "")
   AC_SUBST(LIBSTL_LIBADD, "")])

AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           Enable IPv6 support],
 AC_DEFINE_UNQUOTED(USE_IPV6, , USE_IPV6), )

AC_ARG_ENABLE(dtls,
[  --enable-dtls           Enable DTLS support (requires OpenSSL)],
 [AC_DEFINE_UNQUOTED(USE_DTLS, 1, USE_DTLS)],  )

AC_ARG_ENABLE(pedantic-stack,
[  --enable-pedantic-stack Enable pedantic behavior (fully parse all messages)],
 [AC_DEFINE_UNQUOTED(PEDANTIC_STACK, 1, PEDANTIC_STACK)],  )
 
AM_CONDITIONAL(USE_MYSQL, false)
AC_ARG_WITH(mysql,
[  --with-mysql            Link against MySQL client libraries],
 [AC_DEFINE_UNQUOTED(USE_MYSQL, , USE_MYSQL)
 AC_SUBST(LIBMYSQL_LIBADD, "-lmysqlclient")
 AM_CONDITIONAL(USE_MYSQL, true)], 
 [ AC_SUBST(LIBMYSQL_LIBADD, "")])

# some systems may have a newer version of libpq that is not
# compatible with the packaged version of soci_postgresql
AM_CONDITIONAL(USE_SOCI_POSTGRESQL, false)
AC_ARG_WITH(soci-postgresql,
[  --with-soci-postgresql  Build recon with SOCI PostgreSQL support],
 [AC_DEFINE_UNQUOTED(USE_SOCI_POSTGRESQL, , USE_SOCI_POSTGRESQL)
 AM_CONDITIONAL(USE_SOCI_POSTGRESQL, true)], )

# can't have both MariaDB C client and SOCI MySQL at the same
# time in some environments, e.g. CentOS 8.1
AM_CONDITIONAL(USE_SOCI_MYSQL, false)
AC_ARG_WITH(soci-mysql,
[  --with-soci-mysql       Build recon with SOCI MySQL support],
 [AC_DEFINE_UNQUOTED(USE_SOCI_MYSQL, , USE_SOCI_MYSQL)
 AM_CONDITIONAL(USE_SOCI_MYSQL, true)], )

AM_CONDITIONAL(USE_POSTGRESQL, false)
AC_ARG_WITH(postgresql,
[  --with-postgresql       Link against PostgreSQL client libraries],
 [AC_DEFINE_UNQUOTED(USE_POSTGRESQL, , USE_POSTGRESQL)
 AC_SUBST(LIBPOSTGRESQL_LIBADD, "-lpq")
 AM_CONDITIONAL(USE_POSTGRESQL, true)],
 [ AC_SUBST(LIBPOSTGRESQL_LIBADD, "")])

AM_CONDITIONAL(USE_MAXMIND_GEOIP, false)
AC_ARG_WITH(geoip,
[  --with-geoip            Link against MaxMind GeoIP libraries],
 [AC_DEFINE_UNQUOTED(USE_MAXMIND_GEOIP, , USE_MAXMIND_GEOIP)
 AC_SUBST(LIBGEOIP_LIBADD, "-lGeoIP")
 AM_CONDITIONAL(USE_MAXMIND_GEOIP, true)],
 [ AC_SUBST(LIBGEOIP_LIBADD, "")])

AM_CONDITIONAL(USE_RADIUS_CLIENT, false)
AC_SUBST(LIBRADIUS_LIBADD, "")
AC_ARG_WITH(radius,
[  --with-radius           Link against RADIUS client libraries (ng)],
 [AC_DEFINE_UNQUOTED(USE_RADIUS_CLIENT, , USE_RADIUS_CLIENT)
 AC_SUBST(LIBRADIUS_LIBADD, "-lradiusclient-ng")
 AM_CONDITIONAL(USE_RADIUS_CLIENT, true)])
AC_ARG_WITH(freeradius,
[  --with-freeradius       Link against FreeRADIUS client libraries],
 [AC_DEFINE_UNQUOTED(USE_RADIUS_CLIENT, , USE_RADIUS_CLIENT)
  AC_DEFINE_UNQUOTED(RESIP_HAVE_FREERADIUS_CLIENT, , RESIP_HAVE_FREERADIUS_CLIENT)
 AC_SUBST(LIBRADIUS_LIBADD, "-lfreeradius-client")
 AM_CONDITIONAL(USE_RADIUS_CLIENT, true)])
AC_ARG_WITH(radcli,
[  --with-radcli           Link against radcli RADIUS client library],
 [AC_DEFINE_UNQUOTED(USE_RADIUS_CLIENT, , USE_RADIUS_CLIENT)
  AC_DEFINE_UNQUOTED(RESIP_HAVE_RADCLI, , RESIP_HAVE_RADCLI)
 AC_SUBST(LIBRADIUS_LIBADD, "-lradcli")
 AM_CONDITIONAL(USE_RADIUS_CLIENT, true)])

AM_CONDITIONAL(USE_NETSNMP, false)
AC_ARG_WITH(netsnmp,
[  --with-netsnmp         Link against NetSNMP client libraries],
 [AC_DEFINE_UNQUOTED(USE_NETSNMP, , USE_NETSNMP)
 AC_SUBST(LIBNETSNMP_LDADD, [`net-snmp-config --agent-libs`])
 AM_CONDITIONAL(USE_NETSNMP, true)],
 [ AC_SUBST(LIBNETSNMP_LDADD, "")])

AM_CONDITIONAL(BUILD_REPRO, false)
AC_ARG_WITH(repro,
[  --with-repro            Build repro, requires CAJUN and bdb],
 [AC_DEFINE_UNQUOTED(BUILD_REPRO, , BUILD_REPRO)
 AM_CONDITIONAL(BUILD_REPRO, true)], )

AC_ARG_ENABLE(repro-plugins,
[  --enable-repro-plugins  Enable support for DSO plugins in repro],
 [AC_DEFINE_UNQUOTED(REPRO_DSO_PLUGINS, , REPRO_DSO_PLUGINS)
 AM_CONDITIONAL(BUILD_REPRO_DSO_PLUGINS, true)],
 AM_CONDITIONAL(BUILD_REPRO_DSO_PLUGINS, false))

AC_SUBST([reproplugindir])
reproplugindir='${pkglibdir}/repro/plugins'

AC_SUBST([returnpkglibdir])
returnpkglibdir='${pkglibdir}/reTurnServer'

AM_CONDITIONAL(BUILD_RETURN, false)
AC_ARG_WITH(return,
[  --with-return           Build reTurn client and server],
 [AC_DEFINE_UNQUOTED(BUILD_RETURN, , BUILD_RETURN)
 AM_CONDITIONAL(BUILD_RETURN, true)], )

AM_CONDITIONAL(BUILD_REND, false)
AC_ARG_WITH(rend,
[  --with-rend             Build rend],
 [AC_DEFINE_UNQUOTED(BUILD_REND, , BUILD_REND)
 AM_CONDITIONAL(BUILD_REND, true)], )

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

AM_CONDITIONAL(BUILD_ICHAT_GW, false)
AC_ARG_WITH(ichat-gw,
[  --with-ichat-gw         Build iChat gateway, links against gloox],
 [AC_DEFINE_UNQUOTED(BUILD_ICHAT_GW, , BUILD_ICHAT_GW)
 AM_CONDITIONAL(BUILD_ICHAT_GW, true)], )

AM_CONDITIONAL(BUILD_TELEPATHY_CM, false)
AC_ARG_WITH(telepathy,
[  --with-telepathy        Build the Telepathy Connection Manager],
 [AC_DEFINE_UNQUOTED(BUILD_TELEPATHY_CM, , BUILD_TELEPATHY_CM)
 AM_CONDITIONAL(BUILD_TELEPATHY_CM, true)
 PKG_CHECK_MODULES([QT5], [Qt5Core, Qt5DBus Qt5Network])
 PKG_CHECK_MODULES([TP_QT5], [TelepathyQt5, TelepathyQt5Service])])

AM_CONDITIONAL(BUILD_RECON, false)
AC_ARG_WITH(recon,
[  --with-recon            Build recon],
 [AC_DEFINE_UNQUOTED(BUILD_RECON, , BUILD_RECON)
 AM_CONDITIONAL(BUILD_RECON, true)], )

AM_CONDITIONAL(USE_SRTP1, false)
AC_ARG_WITH(srtp1,
[  --with-srtp1            Use srtp 1.x instead of current version],
 [AC_DEFINE_UNQUOTED(USE_SRTP1, , USE_SRTP1)
 AM_CONDITIONAL(USE_SRTP1, true)], )

AM_CONDITIONAL(BUILD_RECONSERVER, false)

AM_CONDITIONAL(USE_SIPXTAPI, false)
AC_ARG_WITH(sipxtapi,
[  --with-sipxtapi         Build recon with sipXtapi support],
 [AC_DEFINE_UNQUOTED(USE_SIPXTAPI, , USE_SIPXTAPI)
 AM_CONDITIONAL(USE_SIPXTAPI, true)
 AM_CONDITIONAL(BUILD_RECONSERVER, true)], )

AM_CONDITIONAL(USE_KURENTO, false)
AC_ARG_WITH(kurento,
[  --with-kurento          Build recon with Kurento support],
 [AC_DEFINE_UNQUOTED(USE_KURENTO, , USE_KURENTO)
 AM_CONDITIONAL(USE_KURENTO, true)
 AM_CONDITIONAL(BUILD_RECONSERVER, true)], )

AM_CONDITIONAL(RECON_LOCAL_HW_TESTS, false)
AC_ARG_ENABLE(recon-local-hw-tests,
[  --enable-recon-local-hw-tests Attempt to use local audio hardware in unit tests],
 [AC_DEFINE_UNQUOTED(RECON_LOCAL_HW_TESTS, , RECON_LOCAL_HW_TESTS)
 AM_CONDITIONAL(RECON_LOCAL_HW_TESTS, true)
], )

AM_CONDITIONAL(BUILD_P2P, false)
AC_ARG_WITH(p2p,
[  --with-p2p              Build P2P, links against S2C and SSL, unfinished],
 [AC_DEFINE_UNQUOTED(BUILD_P2P, , BUILD_P2P)
 AM_CONDITIONAL(BUILD_P2P, true)], )

AM_CONDITIONAL(BUILD_PYTHON, false)
AC_ARG_WITH(python,
[  --with-python           Build components requiring Python],
 [AC_DEFINE_UNQUOTED(BUILD_PYTHON, , BUILD_PYTHON)
 AM_CONDITIONAL(BUILD_PYTHON, true)
 PKG_CHECK_MODULES([DEPS_PYTHON], [python3 >= 3.6])
 AC_SUBST(DEPS_PYTHON_VERSION)
 AC_SUBST(PYCXX_SRCDIR)
 if [ ! test -e "${PYCXX_SRCDIR}/cxx_extensions.cxx" ];
 then
  echo "Can't find \${PYCXX_SRCDIR}/cxx_extensions.cxx"
  echo "Please check PYCXX_SRCDIR is defined correctly (current value: ${PYCXX_SRCDIR})"
  exit 1
 fi
], )

AM_CONDITIONAL(BUILD_QPID_PROTON, false)
AC_ARG_WITH(qpid-proton,
[  --with-qpid-proton      Build components requiring qpid-proton (AMQP)],
 [AC_DEFINE_UNQUOTED(BUILD_QPID_PROTON, , BUILD_QPID_PROTON)
 AM_CONDITIONAL(BUILD_QPID_PROTON, true)], )

AM_CONDITIONAL(RESIP_ASSERT_SYSLOG, false)
AC_ARG_ENABLE(assert-syslog,
[  --enable-assert-syslog  Log assertion failures with Syslog],
 [AC_DEFINE_UNQUOTED(RESIP_ASSERT_SYSLOG, , RESIP_ASSERT_SYSLOG)
 AM_CONDITIONAL(RESIP_ASSERT_SYSLOG, true)
], )

AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)

#Used to look for realtime library requirement (-lrt)
CURL_CHECK_LIBS_CLOCK_GETTIME_MONOTONIC

AX_HAVE_EPOLL(
  [AC_DEFINE_UNQUOTED(HAVE_EPOLL, 1, HAVE_EPOLL)],  )

AC_CHECK_LIB(dl, dlopen)
AM_CONDITIONAL(HAVE_LIBDL, [test x"$ac_cv_lib_dl_dlopen" = xyes])

AM_CONDITIONAL(BUILD_GSTREAMER, false)
AC_ARG_WITH(gstreamer,
[  --with-gstreamer        Build components requiring GStreamer],
 [AC_DEFINE_UNQUOTED(BUILD_GSTREAMER, , BUILD_GSTREAMER)
 AM_CONDITIONAL(BUILD_GSTREAMER, true)
 PKG_CHECK_MODULES([GSTREAMERMM_1_0], [gstreamermm-1.0])
], )

AM_MAINTAINER_MODE

AC_OUTPUT(Makefile \
	rutil/dns/ares/Makefile \
	rutil/Makefile \
	rutil/test/Makefile \
	resip/Makefile \
	resip/stack/Makefile \
	resip/stack/test/Makefile \
	resip/dum/Makefile \
	resip/dum/test/Makefile \
	resip/certs/Makefile \
	resip/rend/Makefile \
	repro/Makefile \
	repro/accountingconsumers/Makefile \
	repro/reprocmd/Makefile \
	repro/test/Makefile \
	repro/plugins/Makefile \
	repro/plugins/example/Makefile \
	repro/plugins/pyroute/Makefile \
	tfm/Makefile \
	tfm/repro/Makefile \
	tfm/tfdum/Makefile \
	apps/Makefile \
	apps/clicktocall/Makefile \
	apps/clicktocall/test/Makefile \
	apps/echoTest/Makefile \
	apps/ichat-gw/Makefile \
	apps/ichat-gw/jabberconnector/Makefile \
	apps/sipdial/Makefile \
	apps/telepathy/Makefile \
	apps/telepathy/data/Makefile \
	apps/registration-agent/Makefile \
	apps/reConServer/Makefile \
	resiprocate.spec \
	reTurn/Makefile \
	reTurn/test/Makefile \
	reTurn/client/Makefile \
	reTurn/client/test/Makefile \
	reflow/Makefile \
	reflow/dtls_wrapper/test/Makefile \
	resip/recon/Makefile \
	resip/recon/MOHParkServer/Makefile \
	resip/recon/test/Makefile \
	p2p/s2c/s2c/Makefile \
	p2p/Makefile \
	media/Makefile \
	media/kurento/Makefile \
	media/kurento/test/Makefile \
	)


