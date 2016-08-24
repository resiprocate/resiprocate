/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* BUILD_APPS */
#define BUILD_APPS /**/

/* BUILD_ICHAT_GW */
#define BUILD_ICHAT_GW /**/

/* BUILD_P2P */
/* #undef BUILD_P2P */

/* BUILD_PYTHON */
#define BUILD_PYTHON /**/

/* BUILD_RECON */
#define BUILD_RECON /**/

/* BUILD_REND */
/* #undef BUILD_REND */

/* BUILD_REPRO */
#define BUILD_REPRO /**/

/* BUILD_TELEPATHY_CM */
/* #undef BUILD_TELEPATHY_CM */

/* BUILD_TFM */
/* #undef BUILD_TFM */

/* Name of header for libdb */
#define DB_HEADER "db_cxx.h"

/* Define to 1 if you have the clock_gettime function and monotonic timer. */
#define HAVE_CLOCK_GETTIME_MONOTONIC 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* HAVE_EPOLL */
#define HAVE_EPOLL /**/

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* HAVE_POPT_H */
#define HAVE_POPT_H /**/

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

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Major OS version */
#define OS_MAJOR_VER 3

/* Minor OS version */
#define OS_MINOR_VER 16

/* Patch OS version */
#define OS_PATCH_VER 0

/* Point OS version */
#define OS_POINT_VER 0

/* Name of package */
#define PACKAGE "resiprocate"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "resiprocate"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "resiprocate 1.10.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "resiprocate"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.10.0"

/* PEDANTIC_STACK */
/* #undef PEDANTIC_STACK */

/* Host where package was configured */
#define REPRO_BUILD_HOST "localhost"

/* SVN revision */
#define REPRO_BUILD_REV PACKAGE_VERSION

/* REPRO_DSO_PLUGINS */
#define REPRO_DSO_PLUGINS /**/

/* Package version */
#define REPRO_RELEASE_VERSION PACKAGE_VERSION

/* RESIP_ASSERT_SYSLOG */
#define RESIP_ASSERT_SYSLOG /**/

/* RESIP_BIG_ENDIAN */
/* #undef RESIP_BIG_ENDIAN */

/* RESIP_HAVE_FREERADIUS_CLIENT */
/* #undef RESIP_HAVE_FREERADIUS_CLIENT */

/* RESIP_HAVE_RADCLI */
#define RESIP_HAVE_RADCLI /**/

/* Maximum SIP message size to try and parse (bytes) */
#define RESIP_SIP_MSG_MAX_BYTES 10485760

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* USE_ARES */
/* #undef USE_ARES */

/* USE_CARES */
#define USE_CARES /**/

/* USE_DTLS */
#define USE_DTLS 1

/* USE_IPV6 */
#define USE_IPV6 /**/

/* USE_MAXMIND_GEOIP */
/* #undef USE_MAXMIND_GEOIP */

/* USE_MYSQL */
#define USE_MYSQL /**/

/* USE_POSTGRESQL */
#define USE_POSTGRESQL /**/

/* USE_RADIUS_CLIENT */
#define USE_RADIUS_CLIENT /**/

/* USE_SIGCOMP */
/* #undef USE_SIGCOMP */

/* USE_SSL */
#define USE_SSL /**/

/* Version number of package */
#define VERSION "1.10.0"

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
