all: resiprocate tests

.PHONY : resiprocate tests ares contrib


resiprocate: contrib
	cd resiprocate; $(MAKE)

tests: resiprocate
	cd resiprocate/test; $(MAKE)

ares:
	cd contrib/ares; ./configure; $(MAKE)

contrib: ares
