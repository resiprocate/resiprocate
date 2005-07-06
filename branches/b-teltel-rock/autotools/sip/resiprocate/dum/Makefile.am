SUBDIRS = .
CXX = ${DISTCC} @CXX@
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

LIBTOOL = @LIBTOOL@ --tag=CXX

lib_LTLIBRARIES = libresipdum.la
libresipdum_la_SOURCES = \
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
	UInt64Hash.cxx 


libresipdum_la_LDFLAGS = -version-info $(LIBRESIP_SO_VERSION)

resipdum_includedir=$(includedir)/resiprocate $(includedir)/resiprocate/dum
