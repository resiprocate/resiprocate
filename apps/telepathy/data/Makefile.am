# Dbus service file
BUILT_FILES = $(service_in_files:.service.in=.service)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.freedesktop.Telepathy.ConnectionManager.resiprocate.service.in
service_DATA = $(BUILT_FILES)
CLEANFILES = $(BUILT_FILES)

EXTRA_DIST = $(service_in_files)

# Rule to make the service file with libexecdir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@
