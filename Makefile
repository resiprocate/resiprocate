BUILD 	=	build
-include $(BUILD)/Makefile.conf
-include $(BUILD)/Makefile.all
-include $(BUILD)/Makefile.tools
-include $(BUILD)/Makefile.osarch

DEFAULTS :=
ifeq ($(BUILD_RECON), yes)
DEFAULTS += dtls-srtp-openssl
endif
DEFAULTS += stack
ifeq ($(BUILD_REPRO),yes)
DEFAULTS += repro
endif
ifeq ($(BUILD_RECON),yes)
DEFAULTS += recon
endif
ifeq ($(BUILD_RETURN_CLIENT),yes)
DEFAULTS += return-client
endif
ifeq ($(BUILD_RETURN_SERVER),yes)
DEFAULTS += return-server
endif
ifeq ($(BUILD_TFM),yes)
DEFAULTS += tfm
endif

default: $(DEFAULTS)
#default: repro dum tests

stack: dum tests

all: repro dum tests tfm apps recon

tfm: tfmcontrib
	$(MAKE) -C tfm

contrib: 

rutil: contrib
	$(MAKE) -C rutil 

resiprocate: rutil
	$(MAKE) -C resip/stack

dum: resiprocate
	$(MAKE) -C resip/dum

b2bua: resiprocate
	$(MAKE) -C b2bua

repro: dum
	$(MAKE) -C repro

tests: resiprocate 
	$(MAKE) -C rutil/test	
	$(MAKE) -C resip/stack/test
	$(MAKE) -C resip/dum/test	

check: tests
	cd resip/stack/test && ./runtests.sh
	cd rutil/test && ./runtests.sh	

presSvr: resiprocate
	$(MAKE) -C presSvr

apps: dum
	$(MAKE) -C apps

return: return-server return-client

reTurn: return

reTURN: return
	
return-server: rutil
	$(MAKE) -C reTurn

return-client: rutil
	$(MAKE) -C reTurn/client
	$(MAKE) -C reTurn/client/test

recon: dum reflow
	$(MAKE) -C resip/recon
	$(MAKE) -C resip/recon/test

reflow: dtls-srtp-openssl return-client srtp 
	$(MAKE) -C reflow

ifeq (${BUILD_SHARED_LIBS},no)
   NETXX_USE_SHARED_LIBS=--disable-shared
   CPPUNIT_USE_SHARED_LIBS=--enable-shared=false
endif

configure_netxx: tfm/contrib/Netxx-0.3.2/Makefile

tfm/contrib/Netxx-0.3.2/Makefile:
	cd tfm/contrib/Netxx-0.3.2 && CXX="$(CXX)" CXXFLAGS='$(CXXFLAGS)' perl configure.pl --contrib --disable-examples ${NETXX_USE_SHARED_LIBS}

ifeq ($(OSTYPE),MinGW)
netxx:
	$(MAKE) -C tfm/contrib/Netxx-0.3.2 -f Makefile.MinGW
else
netxx: configure_netxx
	$(MAKE) -C tfm/contrib/Netxx-0.3.2 
endif

configure_cppunit: tfm/contrib/cppunit/Makefile
        
tfm/contrib/cppunit/Makefile:
	cd tfm/contrib/cppunit && CC="$(CC)" CFLAGS='$(CFLAGS)' CXX="$(CXX)" CXXFLAGS='$(CXXFLAGS)' ./configure ${CPPUNIT_USE_SHARED_LIBS} ${CONFIGURE_ARGS} --disable-doxygen

cppunit: configure_cppunit
	$(MAKE) -C tfm/contrib/cppunit/src/cppunit libcppunit.la

contrib/srtp/Makefile:
	cd contrib/srtp && ./configure ${CONFIGURE_ARGS}

configure_srtp: contrib/srtp/Makefile

srtp: configure_srtp
	$(MAKE) -C contrib/srtp 

ifneq ($(SSL_LOCATION),)
$(SSL_LOCATION)/Makefile:
	cd $(SSL_LOCATION) && ./Configure linux-generic32 --openssldir=/usr enable-tlsext ${CONFIGURE_ARGS} && $(MAKE) depend

configure_dtls-srtp-openssl: $(SSL_LOCATION)/Makefile

dtls-srtp-openssl: configure_dtls-srtp-openssl
	$(MAKE) -C $(SSL_LOCATION) 
else
dtls-srtp-openssl:
endif

tfmcontrib: cppunit netxx

###########################################################################
# Resiprocate Custom ares rules

ifeq ($(DNS_RESOLVER),resip-ares)

ifeq (${USE_IPV6},yes)
   ARES_IPV6=--with-ipv6
endif

ifeq (${ARES_PREFIX},)
   ARES_PREFIX_ARG=
else
   ARES_PREFIX_ARG=--prefix=${ARES_PREFIX}
endif

contrib/ares-build.$(OS_ARCH)/Makefile:
	mkdir -p contrib/ares-build.$(OS_ARCH)
	cd contrib/ares-build.$(OS_ARCH) && \
	  ../ares/configure ${ARES_IPV6} ${ARES_PREFIX_ARG} ${CONFIGURE_ARGS}

configure_ares: contrib/ares-build.$(OS_ARCH)/Makefile

ares: configure_ares
	$(MAKE) -C contrib/ares-build.$(OS_ARCH) 

contrib: ares

install-ares:
	$(MAKE) -C contrib/ares-build.$(OS_ARCH) install

else
# Dummy rules to use when resip-ares is not being used
install-ares:
endif

clean-ares:
	-rm -Rf contrib/ares-build.*

###########################################################################
# Various clean targets
CLEANDIRS := resip/stack resip/dum resip/dum/test resip/stack/test presSvr \
             repro rutil rutil/test tfm apps reTurn reTurn/client \
             reTurn/client/test p2p p2p/s2c/s2c reflow resip/recon resip/recon/test


cleancontrib: clean-ares
	-$(MAKE) -C tfm/contrib/cppunit distclean
	-$(MAKE) -C tfm/contrib/Netxx-0.3.2 realclean
	find tfm/contrib/Netxx-0.3.2 -name 'Netxx-config' -exec rm -f '{}' \;
	-$(MAKE) -C contrib/srtp superclean
	-$(MAKE) -C contrib/srtp/crypto superclean
	-$(MAKE) -C contrib/srtp/doc superclean
	-$(MAKE) -C contrib/openssl clean

clean: cleanpkg
	for dir in $(CLEANDIRS); do $(MAKE) -C $$dir clean; done ; true

cleanall: cleancontrib
	for dir in $(CLEANDIRS); do $(MAKE) -C $$dir cleanall; done ; true

superclean: cleancontrib cleanpkg
	for dir in $(CLEANDIRS); do $(MAKE) -C $$dir distclean; done ; true
	find * -name '*.db' -exec rm -f '{}' \;
	-rm -Rf .make_prefs
	-rm -f SVN-VERSION
	-rm -Rf lib.*.*

distclean: superclean
	-rm -Rf build/Makefile.conf

###########################################################################
install: install-ares install-rutil install-resip install-dum

install-rutil:
	$(MAKE) -C rutil install

install-resip:
	$(MAKE) -C resip/stack install

install-dum:
	$(MAKE) -C resip/dum install

install-repro:
	$(MAKE) -C repro install

install-recon: install install-reflow install-returnclient
	$(MAKE) -C resip/recon install

install-reflow:
	$(MAKE) -C reflow install

install-returnclient:
	$(MAKE) -C reTurn/client install

SVN-VERSION: 
	@if test -d .svn ; \
	then \
		echo "Generating SVN-VERSION from svnversion"; \
		svnversion . \
		| perl -p \
			-e 'm /(\d+)/ && do { $$padded=sprintf( "%06d", $$1 ); s/\d+/$$padded/; };' \
			-e 's/:/./; s/M/.M/;' \
		> SVN-VERSION ; \
	elif test -r SVN-EXPORT-VERSION ; \
	then \
		echo "Copying SVN-VERSION from SVN-EXPORT-VERSION"; \
		cp SVN-EXPORT-VERSION SVN-VERSION ; \
	else \
		echo "Unknown SVN-VERSION"; \
		echo '0' > SVN-VERSION ; \
	fi
	@echo -n "SVN-VERSION=" ; cat SVN-VERSION; echo ""

REPRO_VERSION = $(shell cat repro/VERSION)

RPMBUILD_TOPDIR = $(shell pwd)/rpm
repro-rpm: repro-dist rpmbuild-area 
	rpmbuild -ta \
		--define="buildno $(shell cat SVN-VERSION)" \
		--define="_topdir $(RPMBUILD_TOPDIR)" \
		repro-$(REPRO_VERSION).tar.gz
	ls -l $(RPMBUILD_TOPDIR)/SRPMS/repro-$(REPRO_VERSION)-*.rpm
	ls -l $(RPMBUILD_TOPDIR)/RPMS/*/repro*-$(REPRO_VERSION)-*.rpm

RPMBUILD_SUBDIRS = BUILD RPMS SOURCES SPECS SRPMS
rpmbuild-area: $(foreach subdir,$(RPMBUILD_SUBDIRS),$(RPMBUILD_TOPDIR)/$(subdir))

$(RPMBUILD_TOPDIR) :
	test -d $(RPMBUILD_TOPDIR) || mkdir $(RPMBUILD_TOPDIR) 

$(foreach subdir,$(RPMBUILD_SUBDIRS),$(RPMBUILD_TOPDIR)/$(subdir)) : $(RPMBUILD_TOPDIR)
	test -d $@ || mkdir $@

repro-dist: cleanpkg repro-$(REPRO_VERSION).tar.gz repro-$(REPRO_VERSION).tar.gz.md5

tmptarfile=/tmp/repro.tar.gz.$$
repro-$(REPRO_VERSION).tar.gz: SVN-VERSION repro.spec 
	rm -f repro-$(REPRO_VERSION)
	ln -s . repro-$(REPRO_VERSION)
	find repro-$(REPRO_VERSION)/ \( -name .svn -prune -o -type f -print0 \) \
	| tar -c -f $(tmptarfile) -z --null -h -T -
	mv $(tmptarfile) $@
	rm -f repro-$(REPRO_VERSION)

repro-$(REPRO_VERSION).tar.gz.md5: repro-$(REPRO_VERSION).tar.gz
	md5sum repro-$(REPRO_VERSION).tar.gz > repro-$(REPRO_VERSION).tar.gz.md5

repro.spec: repro/repro.spec
	$(MAKE) -C repro repro.spec.inst
	mv repro/repro.spec.inst repro.spec

cleanpkg:
	rm -f repro-*.tar.gz repro-*.tar.gz.md5 repro-*.rpm
	rm -rf rpm repro-$(REPRO_VERSION)

# If the make configuration isn't there, create a default one.
$(BUILD)/Makefile.conf:
	./configure -y

.PHONY: resiprocate tests contrib ares srtp dtls-srtp-openssl
.PHONY: install install-ares install-rutil install-resip install-repro install-dum install-reflow install-returnclient install-recon
.PHONY: SVN-VERSION repro-rpm repro-dist cleanpkg rpmbuild-area
.PHONY: repro dum tests tfm tfmcontrib contrib rutil check presSvr

##############################################################################
# 
# The Vovida Software License, Version 1.0 
# Copyright (c) 2000-2007 Vovida Networks, Inc.  All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 
# 3. The names "VOCAL", "Vovida Open Communication Application Library",
#    and "Vovida Open Communication Application Library (VOCAL)" must
#    not be used to endorse or promote products derived from this
#    software without prior written permission. For written
#    permission, please contact vocal@vovida.org.
# 
# 4. Products derived from this software may not be called "VOCAL", nor
#    may "VOCAL" appear in their name, without prior written
#    permission of Vovida Networks, Inc.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
# NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
# IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
# 
# ====================================================================
# 
# This software consists of voluntary contributions made by Vovida
# Networks, Inc. and many individuals on behalf of Vovida Networks,
# Inc.  For more information on Vovida Networks, Inc., please see
# <http://www.vovida.org/>.
# 
##############################################################################
