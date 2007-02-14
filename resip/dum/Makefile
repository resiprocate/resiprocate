# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIP OPENSSL ARES PTHREAD

TARGET_LIBRARY = libdum

TESTPROGRAMS = 

SRC =   \
	AppDialog.cxx \
	AppDialogSet.cxx \
	AppDialogSetFactory.cxx \
	BaseCreator.cxx \
	BaseUsage.cxx \
	UserAuthInfo.cxx \
	BaseSubscription.cxx \
	ChallengeInfo.cxx \
	ClientAuthManager.cxx \
	ClientInviteSession.cxx \
	ClientOutOfDialogReq.cxx \
	ClientPagerMessage.cxx \
	ClientPublication.cxx \
	ClientRegistration.cxx \
	ClientSubscription.cxx \
	DefaultServerReferHandler.cxx \
	DestroyUsage.cxx \
	Dialog.cxx \
	DialogId.cxx \
	DialogSet.cxx \
	DialogSetId.cxx \
	DialogUsage.cxx \
	DialogUsageManager.cxx \
	DumProcessHandler.cxx \
	DumThread.cxx \
	DumTimeout.cxx \
	HandleException.cxx \
	HandleManager.cxx \
	Handled.cxx \
	InMemoryRegistrationDatabase.cxx \
	InviteSession.cxx \
	InviteSessionCreator.cxx \
	InviteSessionHandler.cxx \
	MergedRequestKey.cxx \
	NonDialogUsage.cxx \
	OutOfDialogReqCreator.cxx \
	PagerMessageCreator.cxx \
	MasterProfile.cxx \
	UserProfile.cxx \
	Profile.cxx \
	PublicationCreator.cxx \
	RedirectManager.cxx \
	RegistrationCreator.cxx \
	ServerAuthManager.cxx \
	ServerInviteSession.cxx \
	ServerOutOfDialogReq.cxx \
	ServerPagerMessage.cxx \
	ServerPublication.cxx \
	ServerRegistration.cxx \
	ServerSubscription.cxx \
	SubscriptionHandler.cxx \
	SubscriptionCreator.cxx \
	SubscriptionState.cxx \
	KeepAliveManager.cxx \
	KeepAliveTimeout.cxx \
	NetworkAssociation.cxx \
	EncryptionManager.cxx \
	DumDecrypted.cxx \
	CertMessage.cxx \
	DumFeatureChain.cxx \
	DumFeatureMessage.cxx \
	IdentityHandler.cxx \
	TargetCommand.cxx \
	DumFeature.cxx \
	OutgoingEvent.cxx \
	HttpProvider.cxx \
	HttpGetMessage.cxx \
	DumHelper.cxx \
	MergedRequestRemovalCommand.cxx

include $(BUILD)/Makefile.post
INSTALL_INCDIR := $(DESTDIR)$(INSTALL_PREFIX)/include/resip/dum
