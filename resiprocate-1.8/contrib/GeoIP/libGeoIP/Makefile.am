lib_LTLIBRARIES = libGeoIP.la libGeoIPUpdate.la

EXTRA_DIST = Makefile.vc md5.h global.h types.h GeoIP_internal.h

AM_CPPFLAGS = -DGEOIPDATADIR=\"$(pkgdatadir)\" -Wall

libGeoIP_la_SOURCES = GeoIP.c GeoIPCity.c regionName.c timeZone.c
include_HEADERS = GeoIP.h GeoIPCity.h GeoIPUpdate.h

libGeoIPUpdate_la_SOURCES = GeoIPUpdate.c md5.c

libGeoIP_la_LDFLAGS = -version-info @GEOIP_VERSION_INFO@

libGeoIPUpdate_la_LIBADD = -lz libGeoIP.la

GeoIP.lo GeoIP.o: GeoIP.c GeoIP.h

GeoIPCity.lo GeoIPCity.o: GeoIPCity.c GeoIP.h

GeoIPUpdate.lo GeoIPUpdate.o: GeoIPUpdate.c GeoIPCity.h GeoIP.h

regionName.lo regionName.o: regionName.c

md5.lo md5.o: md5.c

timeZone.lo timeZone.o: timeZone.c

