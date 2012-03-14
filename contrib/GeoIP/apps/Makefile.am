INCLUDES = 	\
	-I$(top_srcdir)/libGeoIP		\
	-Wall

AM_CPPFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\" -Wall

DEPS = $(top_builddir)/libGeoIP/libGeoIP.la
LDADDS = $(top_builddir)/libGeoIP/libGeoIP.la

bin_PROGRAMS = geoiplookup geoiplookup6 geoipupdate

geoiplookup_SOURCES = geoiplookup.c
geoiplookup_LDFLAGS =
geoiplookup_DEPENDENCIES = $(top_builddir)/libGeoIP/libGeoIP.la
geoiplookup_LDADD = $(top_builddir)/libGeoIP/libGeoIP.la

geoiplookup6_SOURCES = geoiplookup6.c
geoiplookup6_LDFLAGS =
geoiplookup6_DEPENDENCIES = $(top_builddir)/libGeoIP/libGeoIP.la
geoiplookup6_LDADD = $(top_builddir)/libGeoIP/libGeoIP.la

geoipupdate_SOURCES = geoipupdate.c
geoipupdate_LDFLAGS =
geoipupdate_DEPENDENCIES = $(top_builddir)/libGeoIP/libGeoIP.la $(top_builddir)/libGeoIP/libGeoIPUpdate.la
geoipupdate_LDADD = $(top_builddir)/libGeoIP/libGeoIPUpdate.la $(top_builddir)/libGeoIP/libGeoIP.la
EXTRA_DIST = geoipupdate-pureperl.pl Makefile.vc
