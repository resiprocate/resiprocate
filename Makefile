# $Id: Makefile,v 1.5 2004/04/11 20:44:34 davidb Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD

TARGET_LIBRARY = libdum

TESTPROGRAMS = 

SRC =   UInt64Hash.cxx \
	DialogUsageManager.cxx \
	BaseUsage.cxx \
	InviteSession.cxx \
	ClientInviteSession.cxx \
	ServerInviteSession.cxx \
	ClientSubscription.cxx \
	ServerSubscription.cxx \
	ClientRegistration.cxx \
	ServerRegistration.cxx \
	ServerPublication.cxx \
	ClientPublication.cxx \
	ServerOutOfDialogReq.cxx \
	ClientOutOfDialogReq.cxx \

include $(BUILD)/Makefile.post
