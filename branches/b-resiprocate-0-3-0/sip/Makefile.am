AUTOMAKE_OPTIONS = foreign

SUBDIRS = resiprocate
EXTRA_DIST = contrib/ares certs README.windows ReSIProcate.sln	\
		SipIMP SipIMP-Qt SipIMPSetup

dist-hook:
	if [ -f $(distdir)/contrib/ares/Makefile ]; then \
		make -C $(distdir)/contrib/ares distclean; \
	fi
	find $(distdir)/contrib $(distdir)/certs $(distdir)/SipIMP \
	  $(distdir)/SipIMP-Qt $(distdir)/SipIMPSetup -name CVS \
	  -o -name .cvsignore | xargs rm -rf

rpms:	dist
	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES/
	cp resip.spec /usr/src/redhat/SPECS/
	rpmbuild -ba /usr/src/redhat/SPECS/resip.spec

srpm:	dist
	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES/
	cp resip.spec /usr/src/redhat/SPECS/
	rpmbuild -bs /usr/src/redhat/SPECS/resip.spec
