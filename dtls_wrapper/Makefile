# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += OPENSSL SRTP

CODE_SUBDIRS = 
TARGET_LIBRARY = libdtls_wrapper
TESTPROGRAMS =

SRC = DtlsTimer.cxx \
	DtlsSocket.cxx \
	DtlsFactory.cxx \
	bf_dwrap.c

include $(BUILD)/Makefile.post

