add_library(libGeoIP STATIC
        libGeoIP/GeoIP.h
        libGeoIP/GeoIP_internal.h
        libGeoIP/GeoIPCity.h
        libGeoIP/pread.h

        libGeoIP/GeoIP.c
        libGeoIP/GeoIP_deprecated.c
        libGeoIP/GeoIPCity.c
        libGeoIP/pread.c
        libGeoIP/regionName.c
        libGeoIP/timeZone.c
)
set_target_properties(libGeoIP PROPERTIES FOLDER contrib)
add_compile_definitions(GEOIP_EXPORTS PACKAGE_VERSION="1.4.8" DLL)
target_include_directories(libGeoIP INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
