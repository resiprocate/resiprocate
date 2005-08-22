
BUILD 	=	build
-include $(BUILD)/Makefile.conf

all: repro dum tests 

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

ifeq (${USE_IPV6},true)
   ARES_IPV6=--with-ipv6
endif

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

