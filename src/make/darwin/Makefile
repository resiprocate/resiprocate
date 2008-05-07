PLATFORM=darwin
NRAPPKIT_SRCDIR=../../../../nrappkit/
NRAPPKIT_BUILD_DIR=$(NRAPPKIT_SRCDIR)src/make/$(PLATFORM)

OPENSSL_SRC_DIR=../../../../openssl-0.9.8g/

CC=gcc -g
GLOBAL_CFLAGS += -DDARWIN -DHAVE_SIN_LEN

include ../generic/pre.mk

include ../generic/targets.mk

GLOBAL_CFLAGS += -DSANITY_CHECKS
