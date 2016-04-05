## Important Changes ##

### 1.6.0 ###

`geoipupdate` is removed from the GeoIP C library.

Please use the seperate tool [geoipupdate](https://github.com/maxmind/geoipupdate) to download your subscriptions or our
free databases.

### 1.5.0 ###

Geoipupdate may be used to download our free databases.

Put this into the config file /usr/local/etc/GeoIP.conf
to download the free GeoLite databases GeoLiteCountry, GeoLiteCity and GeoLiteASNum

```
LicenseKey 000000000000
UserId 999999
ProductIds 506 533 517
```

Free users should create symlinks for the GeoIP databases.
For example:

```
cd /usr/local/share/GeoIP
ln -s GeoLiteCity.dat GeoIPCity.dat
ln -s GeoLiteCountry.dat GeoIPCountry.dat
ln -s GeoLiteASNum.dat GeoIPASNum.dat
```

The lookup functions are thread safe.

### 1.3.6 ###

As of version 1.3.6, the GeoIP C library is thread safe, as long as
`GEOIP_CHECK_CACHE` is not used.

### 1.3.0 ###

The GeoIP Region database is no longer a pointer but an in-structure array so
test the first byte of region == 0 rather than testing if the region pointer
is `NULL`.

### 1.1.0 ###

As of GeoIP 1.1.0 the GeoIP_country_xxx_by_xxx functions return NULL if a
country can not be found. It previously returned `--` or `N/A`. To avoid
segmentation faults, check the return value for `NULL`.
