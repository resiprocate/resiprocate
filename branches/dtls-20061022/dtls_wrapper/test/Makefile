# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += DTLS_WRAPPER RUTIL OPENSSL PTHREAD SRTP ARES

CODE_SUBDIRS = 

SRC = CreateCert.cxx  TestDtlsUdp.cxx TestTimerContext.cxx

TESTPROGRAMS = testDtlsWrapper.cxx testDtlsClient.cxx testDtlsServer.cxx 

include $(BUILD)/Makefile.post

