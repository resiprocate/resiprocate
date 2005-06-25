# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE ARES PTHREAD OPENSSL

CODE_SUBDIRS = external 
TARGET_LIBRARY = librdns
TESTPROGRAMS =

SRC = \
	DnsAAAARecord.cxx \
	DnsCnameRecord.cxx \
	DnsHostRecord.cxx \
	DnsNaptrRecord.cxx \
	DnsSrvRecord.cxx \
	DnsStub.cxx \
	QueryTypes.cxx \
	RRCache.cxx \
	RRList.cxx \
	RROverlay.cxx \
	RRVip.cxx \
	\
	external/HttpProvider.cxx \
	external/HttpGetMessage.cxx \

include $(BUILD)/Makefile.post
