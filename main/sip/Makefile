all: resiprocate tests presSvr

.PHONY : resiprocate tests ares contrib

resiprocate: contrib
	cd resiprocate; $(MAKE)

tests: resiprocate
	cd resiprocate/test; $(MAKE)

presSvr: resiprocate
	cd presSvr; $(MAKE)

ares:
	cd contrib/ares; ./configure; $(MAKE)

contrib: ares

clean: 
	cd resiprocate; $(MAKE) clean
	cd resiprocate/test; $(MAKE) clean
	cd presSvr; $(MAKE) clean
