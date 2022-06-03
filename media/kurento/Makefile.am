

if USE_KURENTO

SUBDIRS = . test

lib_LTLIBRARIES = libkurentoclient.la

libkurentoclient_la_LIBADD = $(LIBSSL_LIBADD)
libkurentoclient_la_LIBADD += @LIBSTL_LIBADD@
libkurentoclient_la_LIBADD += @LIBPTHREAD_LIBADD@
libkurentoclient_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libkurentoclient_la_SOURCES = \
	Event.cxx \
	KurentoConnection.cxx \
	KurentoManager.cxx \
	KurentoSubsystem.cxx \
	Object.cxx

kurentoclientincludedir = $(includedir)/media/kurento
nobase_kurentoclientinclude_HEADERS = \
	Continuation.hxx \
	Event.hxx \
	KurentoConnection.hxx \
	KurentoManager.hxx \
	KurentoResponseHandler.hxx \
	KurentoSubsystem.hxx \
	Object.hxx

endif

