/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* BUILD_APPS */
/* #undef BUILD_APPS */

/* BUILD_GSTREAMER */
/* #undef BUILD_GSTREAMER */

/* BUILD_ICHAT_GW */
/* #undef BUILD_ICHAT_GW */

/* BUILD_P2P */
/* #undef BUILD_P2P */

/* BUILD_PYTHON */
/* #undef BUILD_PYTHON */

/* BUILD_QPID_PROTON */
/* #undef BUILD_QPID_PROTON */

/* BUILD_RECON */
/* #undef BUILD_RECON */

/* BUILD_REND */
/* #undef BUILD_REND */

/* BUILD_REPRO */
/* #undef BUILD_REPRO */

/* BUILD_RETURN */
/* #undef BUILD_RETURN */

/* BUILD_TELEPATHY_CM */
/* #undef BUILD_TELEPATHY_CM */

/* BUILD_TFM */
/* #undef BUILD_TFM */

/* Name of header for libdb */
#define DB_HEADER "db_cxx.h"

/* GPERF size type */
#define GPERF_SIZE_TYPE size_t

/* Define to 1 if you have the clock_gettime function and monotonic timer. */
#define HAVE_CLOCK_GETTIME_MONOTONIC 1

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* HAVE_EPOLL */
#define HAVE_EPOLL 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* HAVE_POPT_H */
/* #undef HAVE_POPT_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if sockaddr_in.sin_len exists */
/* #undef HAVE_sockaddr_in_len */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "resiprocate"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "resiprocate"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "resiprocate 1.13.0~alpha1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "resiprocate"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.13.0~alpha1"

/* PEDANTIC_STACK */
/* #undef PEDANTIC_STACK */

/* RECON_LOCAL_HW_TESTS */
/* #undef RECON_LOCAL_HW_TESTS */

/* Host where package was configured */
#define REPRO_BUILD_HOST "localhost"

/* SVN revision */
#define REPRO_BUILD_REV PACKAGE_VERSION

/* REPRO_DSO_PLUGINS */
/* #undef REPRO_DSO_PLUGINS */

/* Package version */
#define REPRO_RELEASE_VERSION PACKAGE_VERSION

/* RESIP_ASSERT_SYSLOG */
/* #undef RESIP_ASSERT_SYSLOG */

/* RESIP_BIG_ENDIAN */
/* #undef RESIP_BIG_ENDIAN */

/* RESIP_HAVE_FREERADIUS_CLIENT */
/* #undef RESIP_HAVE_FREERADIUS_CLIENT */

/* RESIP_HAVE_RADCLI */
/* #undef RESIP_HAVE_RADCLI */

/* Maximum SIP message size to try and parse (bytes) */
#define RESIP_SIP_MSG_MAX_BYTES 10485760

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* USE_ARES */
#define USE_ARES /**/

/* USE_CARES */
/* #undef USE_CARES */

/* USE_DTLS */
/* #undef USE_DTLS */

/* USE_FMT */
/* #undef USE_FMT */

/* USE_IPV6 */
/* #undef USE_IPV6 */

/* USE_KURENTO */
/* #undef USE_KURENTO */

/* USE_MAXMIND_GEOIP */
/* #undef USE_MAXMIND_GEOIP */

/* USE_MYSQL */
/* #undef USE_MYSQL */

/* USE_NETSNMP */
/* #undef USE_NETSNMP */

/* USE_POSTGRESQL */
/* #undef USE_POSTGRESQL */

/* USE_RADIUS_CLIENT */
/* #undef USE_RADIUS_CLIENT */

/* USE_SIGCOMP */
/* #undef USE_SIGCOMP */

/* USE_SIPXTAPI */
/* #undef USE_SIPXTAPI */

/* USE_SOCI_MYSQL */
/* #undef USE_SOCI_MYSQL */

/* USE_SOCI_POSTGRESQL */
/* #undef USE_SOCI_POSTGRESQL */

/* USE_SRTP1 */
/* #undef USE_SRTP1 */

/* USE_SSL */
/* #undef USE_SSL */

/* Version number of package */
#define VERSION "1.13.0~alpha1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
