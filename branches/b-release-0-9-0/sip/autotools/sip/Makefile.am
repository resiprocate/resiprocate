AUTOMAKE_OPTIONS = foreign

SUBDIRS = resiprocate .
EXTRA_DIST = contrib/ares certs README.windows \
		ReSIProcate_7_0.sln  ReSIProcate_7_1.sln  ReSIProcate_8_0.sln \
		README.autotools sip.mcp.xml

dist-hook:
	if [ -f $(distdir)/contrib/ares/Makefile ]; then \
		${MAKE} -C $(distdir)/contrib/ares distclean; \
	fi
	echo This is a little drastic. Need to re-write to exclude control files.
	find $(distdir) -name .svn -o -name .cvsignore | xargs rm -rf

#rpms:	dist
#	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
#	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES/
#	cp resip.spec /usr/src/redhat/SPECS/
#	rpmbuild -ba /usr/src/redhat/SPECS/resip.spec

#srpm:	dist
#	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
#	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES/
#	cp resip.spec /usr/src/redhat/SPECS/
#	rpmbuild -bs /usr/src/redhat/SPECS/resip.spec
