#
# SYNOPSIS
#
#   AX_RESIP([MINIMUM-VERSION])
#
# NOTE 
#   Currently macros does not check the version of the resiprocate library
#
# DESCRIPTION
#
#   This macro provides tests of availability of the resiprocate library (resiprocate.org)
#
#   Next options are available:
#   --with-resip=path defines the complete path to the Resiprocate includes and 
#                      libraries 
#   --with-resip-inc=path defines the complete path to resip headers
#   --with-resip-lib=path defines the complete path to resip library
#
#   This macro calls:
#
#     AC_SUBST(RESIP_CPPFLAGS)
#     AC_SUBST(RESIP_LDFLAGS)
#     AC_SUBST(RESIP_LIBS)
#
#   And sets:
#
#     HAVE_RESIPROCATE
#
# COPYLEFT
#
#   Copyright (c) 2009 Tarasenko Volodymyr <tvntsr@yahoo.com>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved. This file is 
#   offered as-is, without any warranty.


AC_DEFUN([AX_RESIP],
[
	AC_ARG_WITH([resip],
		AC_HELP_STRING([--with-resip=@<:@ARG@:>@],
			[use Resiprocate library @<:@default=yes@:>@, optionally specify a path to includes and library]
		),
    		[
    			if test "$withval" = "no"; then
 				want_resip="no"
			elif test "$withval" = "yes"; then
				want_resip="yes"
			else
				want_resip="yes"
				resip_path="$withval"
			fi
		],
        	[want_resip="yes"]
	)
	dnl
	dnl RESIP includes
	dnl
	AC_ARG_WITH([resip_inc],
		AC_HELP_STRING([--with-resip-inc=@<:@ARG@:>@],
				[specify Resiprocate includes]
		),
		[
			case "$withval" in
				/* ) ;;
				* )  AC_MSG_ERROR([The Resiprocate includes directory must be an absolute path.]) ;;
			esac

			resip_inc_path="$withval"
		],
		[resip_inc_path="/usr/include /usr/include/resip /usr/local/include /usr/local/resip"]
	)
	dnl
	dnl RESIP libraries
	dnl
	AC_ARG_WITH([resip_lib],
			AC_HELP_STRING([--with-resip-lib=@<:@ARG@:>@],
					[specify Resiprocate library path]
			),
			[
				case "$withval" in
					/* ) ;;
					* )  AC_MSG_ERROR([The Resiprocate library path directory must be an absolute path.]) ;;
				esac

				resip_lib_path="$withval"
			],
			[resip_lib_path="/lib /usr/lib /usr/lib64 /usr/local/lib /usr/local/resip"]
	)

	RESIP_CPPFLAGS=""
	RESIP_LDFLAGS=""
        RESIP_LIBS=""

	dnl
	dnl Do checks
	dnl

	AC_MSG_CHECKING([for Resiprocate library])

	if test "x$want_resip" = "xyes"; then
		AC_REQUIRE([AC_PROG_CPP])
		AC_REQUIRE([AC_CANONICAL_BUILD])

		if test -n "$resip_path"; then
			RESIP_CPPFLAGS="-I$resip_path"
			RESIP_LDFLAGS="-L$resip_path"
		else
                        for inc in $resip_inc_path; do
                                if test -f "$inc/resip/stack/SipStack.hxx"; then
                                        RESIP_CPPFLAGS="-I$inc"
                                        break
                                fi
                        done

			for inc in $resip_lib_path; do
				if test -f "$inc/libresip.so" || test -f "$inc/libresip.a"
				then
					RESIP_LDFLAGS="-L$inc"
					break
				fi
			done
		fi		
        	dnl
	        dnl Simple add libresip and librutil, should be fixed in future
                RESIP_LIBS="-ldum -lresip -lrutil -lares"

		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $RESIP_CPPFLAGS $CFLAGS"

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $RESIP_LDFLAGS"

		LIBS_SAVED="$LIBS"
                LIBS="$RESIP_LIBS $LIBS"
		
		AC_LANG_PUSH(C++)

		AC_LINK_IFELSE(AC_LANG_PROGRAM([[ @%:@include <resip/stack/SipStack.hxx>
                                                                                        ]],
                                  [[

                                    resip::SipStack stack(0, resip::DnsStub::EmptyNameserverList,0,false,0); 
                                    return 0;
                                   ]]),
				[resip_found="yes"],
				[resip_found="no"])

		AC_LANG_POP([C++])

		LDFLAGS="$LDFLAGS_SAVED"
		CPPFLAGS="$CPPFLAGS_SAVED"
		LIBS="$LIBS_SAVED"

		if test "x$resip_found" = "xyes"; then
			AC_DEFINE([HAVE_RESIPROCATE], [1],
					[Define to 1 if RESIPROCATE library is available])

			AC_SUBST(RESIP_CPPFLAGS)

			AC_SUBST(RESIP_LDFLAGS)

                        AC_SUBST(RESIP_LIBS)

			AC_MSG_RESULT([yes])
		else
			AC_MSG_ERROR([[Could not detect the Resiprocate libraries.]])
			AC_MSG_RESULT([no])

		fi
	else
		AC_MSG_RESULT([no])
	fi
])
