AC_DEFUN([RESIP_LIB_OPENSSL],
[
    AC_MSG_CHECKING([for OpenSSL])
    AC_ARG_WITH([openssl],
	AC_HELP_STRING([--with-openssl=DIR], [use the openssl crypto library]),
	[
	    for dir in $with_openssl /usr/local/ssl /usr/ssl /usr/local; do
		if test -f "$dir/include/openssl/ssl.h"; then
		    found_ssl=yes;
		    CPPFLAGS="$CPPFLAGS -I$dir/include"
		    LDFLAGS="$LDFLAGS -L$dir/lib"
		    AC_DEFINE([USE_SSL],[1],[Use OpenSSL])
		    break;
		fi
	    done
	    if test x_$found_ssl != x_yes; then
		if test -f "/usr/include/openssl/ssl.h"; then
		    found_ssl=yes;
		    AC_DEFINE([USE_SSL],[1],[Use OpenSSL])
		fi
	    fi
	    if test x_$found_ssl != x_yes; then
		AC_MSG_ERROR([not found])
	    else
		LIBS="$LIBS -lssl -lcrypto"
		AC_MSG_RESULT([yes])
	    fi
	],
	[ AC_MSG_RESULT([not requested])
	])
])

AC_DEFUN([RESIP_IPV6],
[
        AC_MSG_CHECKING([for ipv6])
        AC_ARG_ENABLE([ipv6],
        AC_HELP_STRING([--enable-ipv6], [use IPv6 code]),
        [
                AC_DEFINE([USE_IPV6],[1],[Select IPv6 code])
                AC_MSG_RESULT([yes])
        ],
        [
                AC_MSG_RESULT([not requested])
        ])
])

AC_DEFUN([RESIP_SCANNER],
[
        AC_MSG_CHECKING([for New Message Scanner])
        AC_ARG_ENABLE([scanner],
        AC_HELP_STRING([--enable-scanner], [use new message scanner]),
        [
                AC_DEFINE([NEW_MSG_HEADER_SCANNER],
                          [1],
                          [Use NewHeaderScanner instead of Preparser])
                AC_MSG_RESULT([yes])
        ],
        [
                AC_MSG_RESULT([not requested])
        ])
])

AC_DEFUN([RESIP_SCANNER_DEBUG],
[
        AC_MSG_CHECKING([for New Message Scanner debug])
        AC_ARG_ENABLE([scanner-debug],
        AC_HELP_STRING([--enable-scanner-debug], [use new message scanner debug]),
        [
                AC_DEFINE([RESIP_MSG_HEADER_SCANNER_DEBUG], [1], [set when debugging requested in MsgHeaderScanner])
                AC_MSG_RESULT([yes])
        ],
        [
                AC_MSG_RESULT([not requested])
        ])
])

                
AC_DEFUN([RESIP_LIB_ARES],
[
    AC_MSG_CHECKING([for ares])
    AC_ARG_WITH([ares],
	AC_HELP_STRING([--with-ares=DIR], [use the ares resolver library]),
	[
	    for dir in $with_ares \
                        `pwd`/contrib/ares \
                        `pwd`/../contrib/ares \
                        /usr/local; do
		if test -f "$dir/include/ares.h"; then
		    found_ares=yes;
		    CPPFLAGS="$CPPFLAGS -I$dir/include"
		    LDFLAGS="$LDFLAGS -L$dir/lib"
		    AC_DEFINE([USE_ARES],[1],[Select ARES Resolver])
		    break;
		fi
		if test -f "$dir/ares.h"; then
		    found_ares=yes;
		    CPPFLAGS="$CPPFLAGS -I$dir"
		    LDFLAGS="$LDFLAGS -L$dir"
		    AC_DEFINE([USE_ARES],[1],[Select ARES Resolver])
		    break;
		fi
	    done
	    if test x_$found_ares != x_yes; then
		if test -f "/usr/include/ares.h"; then
		    found_ares=yes;
		    AC_DEFINE([USE_ARES],[1],[Select ARES Resolver])
		fi
	    fi
	    if test x_$found_ares != x_yes; then
		AC_MSG_ERROR([not found])
	    else
		LIBS="$LIBS -lares"
		AC_MSG_RESULT([yes])
	    fi
	],
	[ AC_MSG_RESULT([not requested])
	])
])

AC_DEFUN([RESIP_PROG_DISTCC],
[
    AC_MSG_CHECKING([for distcc])
    AC_ARG_ENABLE([distcc],
	AC_HELP_STRING([--enable-distcc], [compile in parallel with distcc]),
	[
	    for dir in /usr/local /usr / /usr/local/gnu /usr/gnu /tmp; do
		if test -x "$dir/bin/distcc"; then
		    found_distcc=yes;
		    DISTCC="$dir/bin/distcc"
		    break;
		fi
	    done
	    if test x_$found_distcc != x_yes; then
		AC_MSG_ERROR([not found])
	    else
		AC_MSG_RESULT([yes])
		AC_SUBST([DISTCC])
	    fi
	],
	[ AC_MSG_RESULT([not requested])
	])
])


dnl ## !ah! macro for probing structures for interesting things

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_check_struct_for.html
dnl
AC_DEFUN([AC_CHECK_STRUCT_FOR],[
ac_safe_struct=`echo "$2" | sed 'y%./+-%__p_%'`
ac_safe_member=`echo "$3" | sed 'y%./+-%__p_%'`
ac_safe_all="ac_cv_struct_${ac_safe_struct}_has_${ac_safe_member}"
changequote(, )dnl
  ac_uc_define=STRUCT_`echo "${ac_safe_struct}_HAS_${ac_safe_member}" | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl

AC_MSG_CHECKING([for $2.$3])
AC_CACHE_VAL($ac_safe_all,
[
if test "x$4" = "x"; then
  defineit="= 0"
elif test "x$4" = "xno"; then
  defineit=""
else
  defineit="$4"
fi
AC_TRY_COMPILE([
$1
],[
struct $2 testit;
testit.$3 $defineit;
], eval "${ac_safe_all}=yes", eval "${ac_safe_all}=no" )
])

if eval "test \"x$`echo ${ac_safe_all}`\" = \"xyes\""; then
  AC_MSG_RESULT(yes)
  AC_DEFINE_UNQUOTED($ac_uc_define)
else
  AC_MSG_RESULT(no)
fi
])
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_caolan_func_which_gethostbyname_r.html
dnl
AC_DEFUN(AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R,
[AC_CACHE_CHECK(for which type of gethostbyname_r, ac_cv_func_which_gethostname_r, [
AC_CHECK_FUNC(gethostbyname_r, [
        AC_TRY_COMPILE([
#               include <netdb.h>
        ],      [

        char *name;
        struct hostent *he;
        struct hostent_data data;
        (void) gethostbyname_r(name, he, &data);

                ],ac_cv_func_which_gethostname_r=three,
                        [
dnl                     ac_cv_func_which_gethostname_r=no
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
        char *name;
        struct hostent *he, *res;
        char buffer[2048];
        int buflen = 2048;
        int h_errnop;
        (void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
  ],ac_cv_func_which_gethostname_r=six,

  [
dnl  ac_cv_func_which_gethostname_r=no
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
                        char *name;
                        struct hostent *he;
                        char buffer[2048];
                        int buflen = 2048;
                        int h_errnop;
                        (void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
  ],ac_cv_func_which_gethostname_r=five,ac_cv_func_which_gethostname_r=no)

  ]

  )
                        ]
                )]
        ,ac_cv_func_which_gethostname_r=no)])

if test $ac_cv_func_which_gethostname_r = six; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_6)
elif test $ac_cv_func_which_gethostname_r = five; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_5)
elif test $ac_cv_func_which_gethostname_r = three; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_3)

fi

])
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/acx_pthread.html
dnl
AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*)     flag="-D_THREAD_SAFE";;
                *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD
