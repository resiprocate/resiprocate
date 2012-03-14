pkgdata_DATA =

dist_pkgdata_DATA = GeoIP.dat

DEFAULT_DB_FILE = $(pkgdatadir)/GeoIP.dat

install-data-hook:
	@if test -f "$(DESTDIR)$(DEFAULT_DB_FILE)" ; then \
	        echo "$@ will not overwrite existing $(DESTDIR)$(DEFAULT_DB_FILE)" ; \
	else \
	         	        echo "$(INSTALL_DATA) GeoIP.dat $(DESTDIR)$(DEFAULT_DB_FILE)"; \
				$(INSTALL_DATA) "$(srcdir)/GeoIP.dat" "$(DESTDIR)$(DEFAULT_DB_FILE)"; \
	fi

uninstall-hook:
	@if test -f "$(DESTDIR)$(DEFAULT_DB_FILE)" ; then \
		rm "$(DESTDIR)$(DEFAULT_DB_FILE)"; \
$(INSTALL_DATA) GeoIP.dat $(DESTDIR)$(DEFAULT_DB_FILE); \
	fi
