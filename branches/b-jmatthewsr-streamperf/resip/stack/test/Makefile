# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../../build
CONTRIB = ../../../contrib

include $(BUILD)/Makefile.pre

ifeq ($(USE_DTLS), yes)
PACKAGES += DTLS_SHIM 
TESTPROGRAMS += testshim.cxx
endif

PACKAGES += RESIP RUTIL ARES OPENSSL PTHREAD POPT OPENSIGCOMP

ifeq ($(USE_SSL), yes)
TESTPROGRAMS += makeSelfCert.cxx \
		testSecurity.cxx \
		testTls.cxx 
endif


TESTPROGRAMS += \
testStack.cxx \
testSipMsgEncode.cxx


#testTransportSelector.cxx


SRC = 		TestSupport.cxx

dumpasn1: dumpasn1.c
	gcc -o dumpasn1 dumpasn1.c

include $(BUILD)/Makefile.post
