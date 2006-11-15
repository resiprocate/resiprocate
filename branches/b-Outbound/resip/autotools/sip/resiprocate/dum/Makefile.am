SUBDIRS = . test
CXX = ${DISTCC} @CXX@
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

EXTRA_DIST =	dum_7_0.vcproj \
		dum_7_1.vcproj \
		dum_8_0.vcproj \
		doc Doxyfile

LIBTOOL = @LIBTOOL@ --tag=CXX

lib_LTLIBRARIES = libresipdum.la
libresipdum_la_SOURCES = \
	AppDialog.cxx \
	AppDialogSet.cxx \
	AppDialogSetFactory.cxx \
	BaseCreator.cxx \
	BaseSubscription.cxx \
	BaseUsage.cxx \
        CertMessage.cxx \
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
        DumDecrypted.cxx \
        DumFeatureChain.cxx \
        DumFeature.cxx \
        DumFeatureMessage.cxx \
	DumProcessHandler.cxx \
	DumThread.cxx \
	DumTimeout.cxx \
        EncryptionManager.cxx \
	Handled.cxx \
	HandleException.cxx \
	HandleManager.cxx \
        IdentityHandler.cxx \
	InMemoryRegistrationDatabase.cxx \
	InviteSessionCreator.cxx \
	InviteSession.cxx \
	InviteSessionHandler.cxx \
	KeepAliveManager.cxx \
	KeepAliveTimeout.cxx \
	MasterProfile.cxx \
	MergedRequestKey.cxx \
	NetworkAssociation.cxx \
	NonDialogUsage.cxx \
        OutgoingEvent.cxx \
	OutOfDialogReqCreator.cxx \
	PagerMessageCreator.cxx \
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
	SubscriptionCreator.cxx \
	SubscriptionHandler.cxx \
	SubscriptionState.cxx \
        TargetCommand.cxx \
	UserAuthInfo.cxx \
	UserProfile.cxx


resipdum_include_HEADERS = \
	AppDialog.hxx \
	AppDialogSetFactory.hxx \
	AppDialogSet.hxx \
	BaseCreator.hxx \
	BaseSubscription.hxx \
	BaseUsage.hxx \
        CertMessage.hxx \
	ClientAuthManager.hxx \
	ClientInviteSession.hxx \
	ClientOutOfDialogReq.hxx \
	ClientPagerMessage.hxx \
	ClientPublication.hxx \
	ClientRegistration.hxx \
	ClientSubscription.hxx \
	DefaultServerReferHandler.hxx \
	DestroyUsage.hxx \
	Dialog.hxx \
	DialogId.hxx \
	DialogSetHandler.hxx \
	DialogSet.hxx \
	DialogSetId.hxx \
	DialogUsage.hxx \
	DialogUsageManager.hxx \
        DumCommand.hxx \
        DumDecrypted.hxx \
	DumException.hxx \
        DumFeatureChain.hxx \
        DumFeature.hxx \
        DumFeatureMessage.hxx \
	DumProcessHandler.hxx \
	DumShutdownHandler.hxx \
	DumThread.hxx \
	DumTimeout.hxx \
        EncryptionManager.hxx \
	Handled.hxx \
	HandleException.hxx \
	Handle.hxx \
	HandleManager.hxx \
	Handles.hxx \
        IdentityHandler.hxx \
	InMemoryRegistrationDatabase.hxx \
	InviteSessionCreator.hxx \
	InviteSessionHandler.hxx \
	InviteSession.hxx \
	KeepAliveManager.hxx \
	KeepAliveTimeout.hxx \
	MasterProfile.hxx \
	MergedRequestKey.hxx \
	NetworkAssociation.hxx \
	NonDialogUsage.hxx \
        OutgoingEvent.hxx \
	OutOfDialogHandler.hxx \
	OutOfDialogReqCreator.hxx \
	PagerMessageCreator.hxx \
	PagerMessageHandler.hxx \
	Profile.hxx \
	PublicationCreator.hxx \
	PublicationHandler.hxx \
	RedirectHandler.hxx \
	RedirectManager.hxx \
	RefCountedDestroyer.hxx \
	RegistrationCreator.hxx \
	RegistrationHandler.hxx \
	RegistrationPersistenceManager.hxx \
	RemoteCertStore.hxx \
	ServerAuthManager.hxx \
	ServerInviteSession.hxx \
	ServerOutOfDialogReq.hxx \
	ServerPagerMessage.hxx \
	ServerPublication.hxx \
	ServerRegistration.hxx \
	ServerSubscription.hxx \
	SubscriptionCreator.hxx \
	SubscriptionHandler.hxx \
	SubscriptionPersistenceManager.hxx \
	SubscriptionState.hxx \
        TargetCommand.hxx \
	UsageUseException.hxx \
	UserAuthInfo.hxx \
	UserProfile.hxx

libresipdum_la_LDFLAGS = -version-info $(LIBRESIP_SO_VERSION)

resipdum_includedir=$(includedir)/resiprocate/dum
