AC_DEFUN([RESIP_LIB_RESIP],[

   AC_MSG_CHECKING([for resiprocate])

   AC_ARG_WITH(resip,[
  ---with-resip=path_to_resiprocate],[],[])

   for dir in $with_resip /usr/local /sw /usr; do
     if test -f "$dir/resiprocate/SipStack.hxx"; then
       found_resip=yes;
       CPPFLAGS="$CPPFLAGS -I$dir"
       LDFLAGS="$LDFLAGS -L$dir/resiprocate -L$dir/resiprocate/dum"
       break;
     fi
   done

   if test x_$found_resip != x_yes; then
     AC_MSG_ERROR([not found])
   else
     LIBS="$LIBS -lresiprocate -lresipdum"     
     AC_MSG_RESULT([yes])
   fi

])

AC_DEFUN([RESIP_LIB_OPENSSL],
[
    AC_MSG_CHECKING([for OpenSSL])

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
])

AC_DEFUN([RESIP_SSL_ENABLE],
[
    AC_MSG_CHECKING([for enabling ssl])
    AC_ARG_ENABLE([ssl],
    AC_HELP_STRING([--enable-ssl], 
        [Enable use of SSL @<:@default=yes@:>@ ]),
        [
            if test x_$enableval == x_yes; then
              AC_DEFINE([USE_SSL],[1],[SSL Enabled])
	      CPPFLAGS="$CPPFLAGS -DUSE_SSL"
              AC_MSG_RESULT([SSL Enabled])
	      RESIP_LIB_OPENSSL
            else
              AC_DEFINE([USE_SSL],[0],[SSL Disabled])
              AC_MSG_RESULT([SSL Disabled])
            fi
        ],
        [
            AC_DEFINE([USE_SSL],[1],[SSL Enabled])
	    CPPFLAGS="$CPPFLAGS -DUSE_SSL"
            AC_MSG_RESULT([not requested - enabled by default])
            RESIP_LIB_OPENSSL
        ])
])
        
AC_DEFUN([RESIP_IPV6],
[
        AC_MSG_CHECKING([for ipv6])
        AC_ARG_ENABLE([ipv6],
        AC_HELP_STRING([--enable-ipv6], [use IPv6 code @<:@default=no@:>@ ]),
        [
             if test x_$enableval == x_yes; then
                AC_DEFINE([USE_IPV6],[1],[Select IPv6 code])
	        CPPFLAGS="$CPPFLAGS -DUSE_IPV6"
                AC_MSG_RESULT([yes])
             else
                AC_MSG_RESULT([no])       
             fi
        ],
        [
                AC_MSG_RESULT([not requested])
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

AC_DEFUN([RESIP_CHECK_MYSQL],
[
    AC_MSG_CHECKING([for mysql])
    AC_ARG_ENABLE([mysql],
        AC_HELP_STRING([--enable-mysql], [enable MySQL support]),
	[
	    AC_MSG_RESULT([requested])
            AC_PATH_PROG(mysqlconfig,mysql_config)
            if test [ -z "$mysqlconfig" ]
            then
                AC_MSG_ERROR([mysql_config executable not found])
            else
                AC_MSG_CHECKING(mysql libraries)
                MYSQL_LIBS=`${mysqlconfig} --libs | sed -e \
                    's/-lmysqlclient /-lmysqlclient_r /' -e 's/-lmysqlclient$/-lmysqlclient_r/'`
                AC_MSG_RESULT($MYSQL_LIBS)
                AC_MSG_CHECKING(mysql cflags)
                MYSQL_CFLAGS=`${mysqlconfig} --cflags`
                AC_MSG_RESULT($MYSQL_CFLAGS)
                AC_MSG_CHECKING(mysql includes)
                MYSQL_INCLUDES=`${mysqlconfig} --include`
                AC_MSG_RESULT($MYSQL_INCLUDES)
            fi
	],
	[ AC_MSG_RESULT([not requested])
	])
])

AC_DEFUN([AX_BERKELEY_DB],
[
  old_LIBS="$LIBS"
  AC_ARG_WITH(bdb_cxx_libs,[---with-bdb_cxx_libs=path_to_bdb_cxx_libs],[],[])
  if ! test -z $with_bdb_cxx_libs ; then 
	LDFLAGS="$LDFLAGS -L$with_bdb_cxx_libs"
  fi
  
  minversion=ifelse([$1], ,,$1)

  DB_HEADER=""
  DB_LIBS=""

  if test -z $minversion ; then
      minvermajor=0
      minverminor=0
      minverpatch=0
      AC_MSG_CHECKING([for Berkeley DB])
  else
      minvermajor=`echo $minversion | cut -d. -f1`
      minverminor=`echo $minversion | cut -d. -f2`
      minverpatch=`echo $minversion | cut -d. -f3`
      minvermajor=${minvermajor:-0}
      minverminor=${minverminor:-0}
      minverpatch=${minverpatch:-0}
      AC_MSG_CHECKING([for Berkeley DB >= $minversion])

  fi
	  
	  AC_ARG_WITH(bdb,[---with-bdb=path_to_bdb],[],[])

  for version in 5.0 4.9 4.8 4.7 4.6 4.5 4.4 4.3 4.2 4.1 4.0 3.6 3.5 3.4 3.3 3.2 3.1 ""; do

    if test -z $version ; then
        db_lib="-ldb"
        try_headers="db.h"
    else
        db_lib="-ldb_cxx-$version"
        try_headers="$with_bdb/db_cxx.h db$version/db_cxx.h db`echo $version | sed -e 's,\..*,,g'`/db_cxx.h"
    fi

#    LIBS="$old_LIBS $db_lib"
    LIBS="$db_lib"

    for db_hdr in $try_headers ; do
        if test -z $DB_HEADER ; then
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM(
                    [
                        #include <${db_hdr}>
                    ],
                    [
                        #if !((DB_VERSION_MAJOR > (${minvermajor}) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR > (${minverminor})) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR == (${minverminor}) && \
                                    DB_VERSION_PATCH >= (${minverpatch}))))
                            #error "too old version"
                        #endif

                        DB *db;
                        db_create(&db, NULL, 0);
                    ])],
                [
                    AC_MSG_RESULT([header $db_hdr, library $db_lib])

                    DB_HEADER="$db_hdr"
                    DB_LIBS="$db_lib"
                ])
        fi
    done
  done

  LIBS="$old_LIBS"

  if test -z $DB_HEADER ; then
    AC_MSG_RESULT([not found])
    ifelse([$3], , :, [$3])
  else
    AC_DEFINE_UNQUOTED([DB_HEADER], ["$DB_HEADER"], [Berkeley DB header version])
    AC_SUBST(DB_LIBS)
    ifelse([$2], , :, [$2])
  fi
])
