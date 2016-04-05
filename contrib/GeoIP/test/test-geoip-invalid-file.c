#include "GeoIP.h"

int main()
{
    GeoIP *gi = GeoIP_open(SRCDIR "/README.md", GEOIP_MEMORY_CACHE);

    /* We don't detect invalid files at load, unfortunately. */
    if (gi == NULL) {
        fprintf(stderr, "Error opening database\n");
        return 1;
    }

    const char *country = GeoIP_country_code_by_addr(gi, "24.24.24.24");
    if (country != NULL) {
        fprintf(
            stderr,
            "Received a non-NULL value on an invalid database from GeoIP_country_code_by_addr\n");
        return 1;
    }

    country = GeoIP_country_code_by_addr_v6(gi, "24.24.24.24");
    if (country != NULL) {
        fprintf(
            stderr,
            "Received a non-NULL value on an invalid database from GeoIP_country_code_by_addr_v6\n");
        return 1;
    }

    return 0;
}
