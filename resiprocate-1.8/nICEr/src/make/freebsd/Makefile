PLATFORM=freebsd
NRAPPKIT_SRCDIR=../../../../nrappkit/
NRAPPKIT_BUILD_DIR=$(NRAPPKIT_SRCDIR)src/make/$(PLATFORM)

OPENSSL_SRC_DIR=../../../../openssl-0.9.8g/

#RESIPROCATE_SRCDIR=/home/ekr/c/resiprocate/main/
#RESIPROCATE_PLATFORM=FreeBSD.i386
#RESIPROCATE_DEBUG=debug.

include ../generic/pre.mk

CC=gcc -g
GLOBAL_CFLAGS += -DFREEBSD -DHAVE_SIN_LEN



include ../generic/targets.mk

