# If the ARES_PREFIX make variable is set, it will be passed to ares'
# ./configure as ares' install location.

BUILD 	=	build
-include $(BUILD)/Makefile.conf
-include $(BUILD)/Makefile.all
-include $(BUILD)/Makefile.tools
-include $(BUILD)/Makefile.osarch

all: repro repmgr tests tfm repro_harness repro_soap_perl_client

doc: sm_doc

rutil_doc:
	cd rutil/ && doxygen Doxyfile;

stack_doc: rutil_doc rutil_doxytag
	cd resip/stack && doxygen Doxyfile

sm_doc: stack_doc stack_doxytag
	cd resip/ReparteeManager && doxygen Doxyfile

rutil_doxytag:
	cd rutil/doxygen/html && doxytag -t rutildox.tag

stack_doxytag:
	cd resip/stack/doxygen/html && doxytag -t fsdkdox.tag

sm_doxytag:
	cd resip/ReparteeManager/doxygen/html && doxytag -t RepMgrdox.tag

repro_soap_perl_client: contrib rutil gsoap
	($(MAKE) -C repro/SOAPOAM perl_wrapper)

install-repro_soap_perl_client:
	($(MAKE) -C repro/SOAPOAM install_perl_wrapper)

gsoap: repro-deps/gsoap-2.7/soapcpp2/libgsoap++.a 
	echo $(ROOT)
	
repro-deps/gsoap-2.7/soapcpp2/libgsoap++.a: repro-deps/gsoap-2.7/Makefile build/Makefile.conf
	( $(MAKE) -C repro-deps/gsoap-2.7 )
	sed -i -e 's,-lgsoap++$$,$${libdir}/libgsoap++.a,' repro-deps/gsoap-2.7/gsoap++.pc

repro-deps/gsoap-2.7/Makefile: repro-deps/gsoap-2.7/configure
	( cd repro-deps/gsoap-2.7 && ./configure )

repro-deps/gsoap-2.7/configure:
	cd repro-deps; tar xf gsoap_2.7.8c_trimmed.tar.gz ;  ./patch_gsoap.sh gsoap-2.7

tfm: tfmcontrib
	$(MAKE) -C tfm

rutil: contrib
	$(MAKE) -C rutil

resiprocate: rutil
	$(MAKE) -C resip/stack

#if, for some reason, external is not present, bail
repmgr: resiprocate
	$(MAKE) -C resip/ReparteeManager

repro: repmgr reprodeps
	$(MAKE) -C repro
	
tests: repmgr
	$(MAKE) -C rutil/test
	$(MAKE) -C resip/stack/test
	$(MAKE) -C resip/ReparteeManager/test
	
check: tests
	cd resip/ReparteeManager/test && ./runtests.sh
	cd resip/stack/test && ./runtests.sh
	cd rutil/test && ./runtests.sh

ifeq (${BUILD_SHARED_LIBS},no)
   NETXX_USE_SHARED_LIBS=--disable-shared
   CPPUNIT_USE_SHARED_LIBS=--enable-shared=false
endif

ifeq (${USE_IPV6},yes)
   ARES_IPV6=--with-ipv6
endif

# Setting the ARES_PREFIX make variable means that its value should be
# passed to ares' ./configure as ares' install prefix.
# Here we construct the ARES_PREFIX_ARG make variable, which is either empty
# or the appropriate --prefix option for ares' ./configure.
ifeq (${INSTALL_PREFIX},)
   ARES_PREFIX_ARG=
else
   ARES_PREFIX_ARG=--prefix=${INSTALL_PREFIX} --exec-prefix=${INSTALL_PREFIX}
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
	cd tfm/contrib/cppunit && CC="$(CC)" CFLAGS='$(CFLAGS)' CXX="$(CXX)" CXXFLAGS='$(CXXFLAGS)' ./configure ${CPPUNIT_USE_SHARED_LIBS} ${CONFIGURE_ARGS}

cppunit: configure_cppunit
	$(MAKE) -C tfm/contrib/cppunit/src/cppunit libcppunit.la

repro_harness: repro tfm
	$(MAKE) -C tfm/repro

remoteproxy_harness: tfm
	$(MAKE) -C tfm/remoteproxy

sipbasis_harness: tfm
	$(MAKE) -C tfm/sipbasis

# If we are building ares under Resiprocate, ares needs to know the
# Resiprocate install directory.  It is passed in via the ARES_PREFIX
# make variable.
contrib/ares-build.$(OS_ARCH)/Makefile:
	mkdir -p contrib/ares-build.$(OS_ARCH)
	cd contrib/ares-build.$(OS_ARCH) && \
	  ../ares/configure ${ARES_IPV6} ${ARES_PREFIX_ARG} ${CONFIGURE_ARGS} --enable-shared=$(BUILD_SHARED_LIBS)

configure_ares: contrib/ares-build.$(OS_ARCH)/Makefile

ares: configure_ares
	$(MAKE) -C contrib/ares-build.$(OS_ARCH)

contrib/dtls/Makefile:
	cd contrib/dtls && ./config

configure_dtls: contrib/dtls/Makefile

dtls: configure_dtls
	$(MAKE) -C contrib/dtls

tfmcontrib: cppunit netxx

# Only build contrib/ares if we need it
ifeq ($(DNS_RESOLVER),resip-ares)
contrib: ares
else
contrib:
endif

reprodeps: gsoap

reprodeps-distclean:
	rm -rf repro-deps/gsoap-2.7

	
	
###########################################################################
# Various clean targets
CLEANDIRS := resip/stack resip/ReparteeManager resip/ReparteeManager/test resip/stack/test \
              repro repro/SOAPOAM rutil rutil/test tfm tfm/repro tfm/remoteproxy\
               tfm/sipbasis


cleancontrib: reprodeps-distclean
	-rm -Rf contrib/ares-build.*
	-$(MAKE) -C tfm/contrib/cppunit distclean
	-$(MAKE) -C tfm/contrib/Netxx-0.3.2 realclean
	find tfm/contrib/Netxx-0.3.2 -name 'Netxx-config' -exec rm -f '{}' \;

clean: 
	for dir in $(CLEANDIRS); do $(MAKE) -C $$dir clean; done ; true

cleanall: superclean

# For backwards compat
rpm_clean: clean-rpm

superclean: cleancontrib clean-rpm
	for dir in $(CLEANDIRS); do $(MAKE) -C $$dir distclean; done ; true
	find * -name '*.db' -exec rm -f '{}' \;
	-rm -Rf .make_prefs
	-rm -f SVN-VERSION
	-rm -Rf lib.*.*

distclean: superclean
	-rm -Rf build/Makefile.conf

###########################################################################

# install does not include install-ares, because it did not in the
# past.  (As far as I know, installing ares is needed only when
# resiprocateLib is used as part of sipX, and the sipX Makefiles
# invoke the install-ares target directly.)
# .bwc. We need ares if we are installing shared libraries.
install: install-ares install-rutil install-resip install-repmgr

# Only install ares if we need it
ifeq ($(DNS_RESOLVER),resip-ares)
install-ares: configure_ares
	$(MAKE) -C contrib/ares-build.$(OS_ARCH)
else
install-ares: 
endif

install-rutil: install-ares
	$(MAKE) -C rutil install

install-resip:
	$(MAKE) -C resip/stack install

install-repmgr:
	$(MAKE) -C resip/ReparteeManager install

install-repro:
	$(MAKE) -C repro install
	
install-oamp-client:
	$(MAKE) -C repro install-oamp-client

# See rpmbuild/Makefile for valid targets
# Right now they are stack-rpm, swProxy-rpms, repro-rpm, and oamp-client-rpm
%-rpm:
	$(MAKE) -C rpmbuild $@
	cp rpmbuild/*.rpm . # Maybe have this only copy the rpms that were just built

%-rpms:
	$(MAKE) -C rpmbuild $@
	cp rpmbuild/*.rpm . # Maybe have this only copy the rpms that were just built

%-rpm-ignore-diffs:
	$(MAKE) -C rpmbuild $@
	cp rpmbuild/*.rpm . # Maybe have this only copy the rpms that were just built

%-rpms-ignore-diffs:
	$(MAKE) -C rpmbuild $@
	cp rpmbuild/*.rpm . # Maybe have this only copy the rpms that were just built

# If the make configuration isn't there, create a default one.
$(BUILD)/Makefile.conf:
	./configure -y

.PHONY: resiprocate tests contrib ares dtls
.PHONY: install install-ares install-rutil install-resip install-repro install-repmgr
.PHONY: repro repmgr tests tfm tfmcontrib rutil check gsoap

# Copyright 2007 Estacado Systems #

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
