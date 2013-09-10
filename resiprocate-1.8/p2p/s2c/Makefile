# nrappkit is Network Resonance's application development kit.
# See http://nrappkit.sourceforge.net

NRAPPKIT_SRC = /users/ekr/src/nrappkit/src/
NRAPPKIT_MAKE=$(NRAPPKIT_SRC)make/freebsd

#NRAPPKIT_SRC = /Users/ekr/src/nrappkit/src/
#NRAPPKIT_MAKE=$(NRAPPKIT_SRC)make/darwin


RESIPROCATE_SRCDIR=/users/ekr/src/resiprocate/main/
RESIPROCATE_PLATFORM=FreeBSD.i386
RESIPROCATE_DEBUG=debug.

YFLAGS	= -dv
CFLAGS	= -g -DYYDEBUG=0 -I$(NRAPPKIT_MAKE) -I$(NRAPPKIT_SRC)util/libekr
CFLAGS += -DHAVE_STRDUP
LDFLAGS	= -L$(NRAPPKIT_MAKE) -lnrappkit


SRC	= gram.y scan.l 
OBJ	= gram.o scan.o gen.o

s2c:	main.o $(OBJ)
	cc $(CFLAGS) -g -o $@ main.o $(OBJ) $(LDFLAGS)

test:   test_driver.o test.o s2c_native.o 
	cc $(CFLAGS) -g -o $@ test_driver.o test.o s2c_native.o $(LDFLAGS)

$(OBJ): y.tab.h

y.tab.h: gram.c gram.y

scan.c  : y.tab.h
scan.o	: y.tab.h

clean	:
	rm -f y.tab.h y.output *.o clic gram.c


