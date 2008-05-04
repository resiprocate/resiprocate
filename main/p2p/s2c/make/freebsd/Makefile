PLATFORM=freebsd
CC=gcc -g
BINARY_PATH=$(PWD)/

#all: programs
include ../generic/pre.mk

CFLAGS += -DSANITY_CHECKS -Wno-unused  -DNDEBUG -DFREEBSD
CFLAGS += -fPIC
CFLAGS += -DHAVE_STRLCPY  # cleanup for libpcap
CFLAGS += -DHAVE_SIN_LEN
CFLAGS += -DDEBUG

GLOBAL_LDFLAGS += -Wl,--export-dynamic

include ../generic/targets.mk

