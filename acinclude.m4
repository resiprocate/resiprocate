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
