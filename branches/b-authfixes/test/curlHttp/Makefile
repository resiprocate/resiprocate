# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../../build
CONTRIB = ../../../contrib

include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL PTHREAD POPT CURL

ifeq ($(USE_SSL), yes)
TESTPROGRAMS +=
endif

TESTPROGRAMS += testCurlHttpProvider.cxx

#TARGET_LIBRARY = curlShit

SRC = 		CurlHttpProvider.cxx

dumpasn1: dumpasn1.c
	gcc -o dumpasn1 dumpasn1.c

include $(BUILD)/Makefile.post
