AM_CPPFLAGS = -Wall -ansi
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

SUBDIRS =		\
	libGeoIP	\
	apps		\
	test		\
	man

pkgconfigdir = $(libdir)/pkgconfig/
nodist_pkgconfig_DATA = geoip.pc

EXTRA_DIST = LICENSE NEWS.md README.md README.OSX bootstrap GeoIP.spec GeoIP.spec.in Makefile.vc get_ver.awk geoip.ico geoip.pc.in data/GeoIP.dat
