# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL ARES PTHREAD

TARGET_LIBRARY = libdum

TESTPROGRAMS = 

SRC =   \
	AppDialog.cxx \
	AppDialogSet.cxx \
	BaseCreator.cxx \
	BaseUsage.cxx \
	ClientAuthManager.cxx \
	ClientInviteSession.cxx \
	ClientOutOfDialogReq.cxx \
	ClientPublication.cxx \
	ClientRegistration.cxx \
	ClientSubscription.cxx \
	Dialog.cxx \
	DialogId.cxx \
	DialogSet.cxx \
	DialogSetId.cxx \
	DialogUsageManager.cxx \
	DumTimeout.cxx \
	HandleException.cxx \
	HandleManager.cxx \
	Handled.cxx \
	InviteSession.cxx \
	InviteSessionCreator.cxx \
	InviteSessionHandler.cxx \
	MergedRequestKey.cxx \
	OutOfDialogReqCreator.cxx \
	Profile.cxx \
	PublicationCreator.cxx \
	RegistrationCreator.cxx \
	ServerAuthManager.cxx \
	ServerInviteSession.cxx \
	ServerOutOfDialogReq.cxx \
	ServerPublication.cxx \
	ServerRegistration.cxx \
	ServerSubscription.cxx \
	SubscriptionCreator.cxx \
	UInt64Hash.cxx \

include $(BUILD)/Makefile.post
