SUBDIRS = .
CXX = ${DISTCC} @CXX@
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

LIBTOOL = @LIBTOOL@ --tag=CXX

lib_LTLIBRARIES = libresipdum.la
libresipdum_la_SOURCES = \
	AppDialog.cxx \
	AppDialogSet.cxx \
	AppDialogSetFactory.cxx \
	BaseCreator.cxx \
	BaseSubscription.cxx \
	BaseUsage.cxx \
	ClientAuthManager.cxx \
	ClientInviteSession.cxx \
	ClientOutOfDialogReq.cxx \
	ClientPagerMessage.cxx \
	ClientPublication.cxx \
	ClientRegistration.cxx \
	ClientSubscription.cxx \
	DefaultServerReferHandler.cxx \
	Dialog.cxx \
	DialogId.cxx \
	DialogSet.cxx \
	DialogSetId.cxx \
	DialogUsage.cxx \
	DialogUsageManager.cxx \
	DumTimeout.cxx \
	Handle.cxx \
	Handled.cxx \
	HandleException.cxx \
	HandleManager.cxx \
	InMemoryRegistrationDatabase.cxx \
	InviteSessionCreator.cxx \
	InviteSession.cxx \
	InviteSessionHandler.cxx \
	MergedRequestKey.cxx \
	NonDialogUsage.cxx \
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
	UInt64Hash.cxx 


libresipdum_la_LDFLAGS = -version-info $(LIBRESIP_SO_VERSION)

resipdum_includedir=$(includedir)/resiprocate $(includedir)/resiprocate/dum
