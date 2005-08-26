
BUILD 	=	build
-include $(BUILD)/Makefile.conf

all: repro dum tests tfm

tfm: tfmcontrib
	cd tfm; $(MAKE)

resiprocate: contrib 
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

ifeq (${USE_IPV6},true)
   ARES_IPV6=--with-ipv6
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

contrib/ares/Makefile:
	cd contrib/ares && ./configure ${ARES_IPV6}

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

install:
	cd rutil; $(MAKE) install
	cd resip/stack; $(MAKE) install
	cd resip/dum; $(MAKE) install
	cd repro; $(MAKE) install

.PHONY : resiprocate tests contrib ares dtls

