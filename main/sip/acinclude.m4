AC_DEFUN([RESIP_LIB_OPENSSL],
[
    AC_MSG_CHECKING([for OpenSSL])
    AC_ARG_WITH([openssl],
        AC_HELP_STRING([--with-openssl=DIR], [use the openssl crypto library]),
        [
            for dir in $with_openssl /usr/local/ssl /usr/ssl /usr/local /usr; do
                if test -f "$dir/include/openssl/ssl.h"; then
                    found_ssl=yes;
                    CPPFLAGS="$CPPFLAGS -I$dir/include"
                    LDFLAGS="$LDFLAGS -L$dir/lib"
                    AC_DEFINE([USE_SSL])
                    break;
                fi
            done
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

AC_DEFUN([RESIP_LIB_ARES],
[
    AC_MSG_CHECKING([for ares])
    AC_ARG_WITH([ares],
        AC_HELP_STRING([--with-ares=DIR], [use the ares resolver library]),
        [
	    for dir in $with_ares `pwd`/contrib/ares /usr/local /usr; do
		if test -f "$dir/include/ares.h"; then
		    found_ares=yes;
		    CPPFLAGS="$CPPFLAGS -I$dir/include"
                    LDFLAGS="$LDFLAGS -L$dir/lib"
		    AC_DEFINE([USE_ARES])
		    break;
		fi
		if test -f "$dir/ares.h"; then
		    found_ares=yes;
		    CPPFLAGS="$CPPFLAGS -I$dir"
                    LDFLAGS="$LDFLAGS -L$dir"
		    AC_DEFINE([USE_ARES])
		    break;
		fi
	    done
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
