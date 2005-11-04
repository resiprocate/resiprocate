# If the ARES_PREFIX make variable is set, it will be passed to ares'
# ./configure as ares' install location.

BUILD 	=	build
-include $(BUILD)/Makefile.conf

all: repro dum tests tfm

tfm: tfmcontrib
	cd tfm; $(MAKE)

rutil: contrib
	cd rutil; $(MAKE) 

resiprocate: rutil
	cd resip/stack; $(MAKE)

dum: resiprocate
	cd resip/dum; $(MAKE)

repro: dum
	cd repro; $(MAKE)

tests: resiprocate 
	cd resip/stack/test; $(MAKE)

presSvr: resiprocate
	cd presSvr; $(MAKE)

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
ifeq (${ARES_PREFIX},)
   ARES_PREFIX_ARG=
else
   ARES_PREFIX_ARG=--prefix=${PREFIX}
endif

configure_netxx: tfm/contrib/Netxx-0.3.2/Makefile

tfm/contrib/Netxx-0.3.2/Makefile:
	cd tfm/contrib/Netxx-0.3.2 && perl configure.pl --contrib --disable-examples ${NETXX_USE_SHARED_LIBS}

netxx: configure_netxx
	cd tfm/contrib/Netxx-0.3.2 && $(MAKE)

configure_cppunit: tfm/contrib/cppunit/Makefile

tfm/contrib/cppunit/Makefile:
	cd tfm/contrib/cppunit && ./configure ${CPPUNIT_USE_SHARED_LIBS}

cppunit: configure_cppunit
	cd tfm/contrib/cppunit && $(MAKE)
configure_ares: contrib/ares/Makefile

# If we are building ares under Resiprocate, ares needs to know the
# Resiprocate install directory.  It is passed in via the ARES_PREFIX
# make variable.
contrib/ares/Makefile:
	cd contrib/ares && ./configure ${ARES_IPV6} ${ARES_PREFIX_ARG}

configure_ares: contrib/ares/Makefile

ares: configure_ares
	cd contrib/ares && $(MAKE)

contrib/dtls/Makefile:
	cd contrib/dtls && ./config

configure_dtls: contrib/dtls/Makefile

dtls: configure_dtls
	cd contrib/dtls && $(MAKE)

tfmcontrib: cppunit netxx

contrib: ares 

clean: 
	cd resip/stack; $(MAKE) clean
	cd resip/dum; $(MAKE) clean
	cd resip/stack/test; $(MAKE) clean
	cd presSvr; $(MAKE) clean

# install does not include install-ares, because it did not in the
# past.  (As far as I know, installing ares is needed only when
# resiprocateLib is used as part of sipX, and the sipX Makefiles
# invoke the install-ares target directly.)
install: install-rutil install-resip install-dum install-repro

install-ares:
	cd contrib/ares; $(MAKE) install

install-rutil:
	cd rutil; $(MAKE) install

install-resip:
	cd resip/stack; $(MAKE) install

install-dum:
	cd resip/dum; $(MAKE) install

install-repro:
	cd repro; $(MAKE) install

.PHONY : resiprocate tests contrib ares dtls
.PHONY : install install-ares install-rutil install-resip install-repro install-dum
