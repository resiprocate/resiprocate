
BUILD 	=	build
-include $(BUILD)/Makefile.conf

all: repro dum tests 

resiprocate: contrib 
	cd resiprocate; $(MAKE)

dum: resiprocate
	cd resiprocate/dum; $(MAKE)

repro: dum
	cd repro; $(MAKE)

tests: resiprocate 
	cd resiprocate/test; $(MAKE)

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
	cd resiprocate; $(MAKE) clean
	cd resiprocate/test; $(MAKE) clean
	cd presSvr; $(MAKE) clean

.PHONY : resiprocate tests contrib ares dtls

