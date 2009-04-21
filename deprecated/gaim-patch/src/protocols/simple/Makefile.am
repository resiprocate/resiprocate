
pkgdir = $(libdir)/gaim

SIMPLESOURCES =		  simple.c \
			  simple.h 

AM_CFLAGS = $(st)

libsimple_la_LDFLAGS = -module -avoid-version

if STATIC_SIMPLE

st = -DSTATIC
noinst_LIBRARIES = libsimple.a
pkg_LTLIBRARIES =

libsimple_a_SOURCES = $(SIMPLESOURCES)
libsimple_a_CFLAGS  = $(AM_CFLAGS)

else

st =
pkg_LTLIBRARIES = libsimple.la
noinst_LIBRARIES =

libsimple_la_SOURCES = $(SIMPLESOURCES)

endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(DEBUG_CFLAGS) \
	$(GLIB_CFLAGS)
