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
