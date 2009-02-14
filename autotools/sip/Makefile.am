AUTOMAKE_OPTIONS = foreign

RPMSOURCEDIR    = $(shell rpm --eval '%_sourcedir')
RPMSPECDIR      = $(shell rpm --eval '%_specdir')
RPMSRCRPMDIR    = $(shell rpm --eval '%_srcrpmdir')

SUBDIRS = resiprocate

EXTRA_DIST = contrib/ares certs README.windows ReSIProcate.sln	\
		SipIMP SipIMP-Qt SipIMPSetup ReSIProcate_7_1.sln \
		sip.xcode README.autotools

dist-hook:
	if [ -f $(distdir)/contrib/ares/Makefile ]; then \
		make -C $(distdir)/contrib/ares distclean; \
	fi
	echo This is a little drastic. Need to re-write to exclude control files.
	find $(distdir)/contrib $(distdir)/certs $(distdir)/SipIMP \
	     $(distdir)/SipIMP-Qt $(distdir)/SipIMPSetup -name CVS \
             -o -name .cvsignore | xargs rm -rf

rpms:	dist
	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
	cp $(PACKAGE)-$(VERSION).tar.gz $(RPMSOURCEDIR)/
	cp resip.spec $(RPMSPECDIR)/
	rpmbuild -ba $(RPMSPECDIR)/resip.spec

srpm:	dist
	sed -e "s/RESIP_VERSION/$(VERSION)/g" -e "s/RESIP_RELEASE/$(RELEASE)/g" < resip.spec.in > resip.spec
	cp $(PACKAGE)-$(VERSION).tar.gz $(RPMSOURCEDIR)/
	cp resip.spec $(RPMSPECDIR)/
	rpmbuild -bs $(RPMSPECDIR)/resip.spec

i686:	srpm
	rpmbuild --rebuild --target i686 $(RPMSRCRPMDIR)/$(PACKAGE)-$(VERSION)-$(RELEASE).src.rpm
