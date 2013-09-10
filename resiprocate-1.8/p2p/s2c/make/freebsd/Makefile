PLATFORM=freebsd
CC=gcc -g
CXX=g++ -g
BINARY_PATH=$(PWD)/

RESIPROCATE_SRCDIR=/users/ekr/src/resip/main/
RESIPROCATE_PLATFORM=FreeBSD.i386
RESIPROCATE_DEBUG=debug.

#all: programs
include ../generic/pre.mk

CFLAGS += -DSANITY_CHECKS -Wno-unused  -DNDEBUG -DFREEBSD
CFLAGS += -fPIC
CFLAGS += -DHAVE_STRLCPY  # cleanup for libpcap
CFLAGS += -DHAVE_SIN_LEN
CFLAGS += -DDEBUG

GLOBAL_LDFLAGS += -Wl,--export-dynamic

include ../generic/targets.mk

