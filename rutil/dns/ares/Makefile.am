# $Id: $
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = libresipares.la
libresipares_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic
if MACOSX
libresipares_la_LDFLAGS += -framework SystemConfiguration
endif
libresipares_la_SOURCES = \
	ares__close_sockets.c ares__get_hostent.c ares__read_line.c \
	ares_destroy.c ares_expand_name.c ares_fds.c ares_free_errmem.c \
	ares_free_hostent.c ares_free_string.c ares_gethostbyaddr.c \
	ares_gethostbyname.c ares_init.c ares_mkquery.c ares_parse_a_reply.c \
	ares_parse_ptr_reply.c ares_process.c ares_query.c ares_search.c \
	ares_send.c ares_strerror.c ares_timeout.c ares_local.c

libresipares_ladir = $(includedir)/rutil/dns/ares
libresipares_la_HEADERS =\
	ares.h ares_compat.h ares_dns.h ares_local.h ares_socketfunc.h ares_version.h

noinst_HEADERS = ares_private.h
man3_MANS = \
	ares_destroy.3          ares_free_hostent.3   ares_init.3  \
	ares_parse_ptr_reply.3  ares_send.3           ares_expand_name.3 \
	ares_free_string.3      ares_init_options.3   ares_process.3  \
	ares_strerror.3         ares_fds.3            ares_gethostbyaddr.3  \
	ares_mkquery.3          ares_query.3          ares_timeout.3 \
	ares_free_errmem.3      ares_gethostbyname.3  ares_parse_a_reply.3  \
	ares_search.3

EXTRA_DIST = $(man3_MANS)
EXTRA_DIST += *.vcxproj
EXTRA_DIST += *.vcxproj.filters

EXTRA_PROGRAMS = adig ahost aresfuzz aresfuzzname

adig_SOURCES = adig.c
adig_LDADD = libresipares.la

ahost_SOURCES = ahost.c
ahost_LDADD = libresipares.la

aresfuzz_SOURCES = ares-test-fuzz.c
aresfuzz_LDFLAGS = ${LIB_FUZZING_ENGINE}
aresfuzz_LDADD = libresipares.la
nodist_EXTRA_aresfuzz_SOURCES = dummy.cpp # Enforce C++ linking

aresfuzzname_SOURCES = ares-test-fuzz-name.c
aresfuzzname_LDFLAGS = ${LIB_FUZZING_ENGINE}
aresfuzzname_LDADD = libresipares.la
nodist_EXTRA_aresfuzzname_SOURCES = dummy.cpp # Enforce C++ linking

#distclean: clean
#	rm -f config.cache config.log config.status Makefile
