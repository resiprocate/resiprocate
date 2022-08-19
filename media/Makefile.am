
SUBDIRS = kurento
SUBDIRS += gstreamer

lib_LTLIBRARIES = libresipmedia.la

libresipmedia_la_LIBADD = ../resip/stack/libresip.la
libresipmedia_la_LIBADD += ../rutil/librutil.la
libresipmedia_la_LIBADD += @LIBSTL_LIBADD@
libresipmedia_la_LIBADD += @LIBPTHREAD_LIBADD@
libresipmedia_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libresipmedia_la_SOURCES = \
        RTPPortManager.cxx

resipmediaincludedir = $(includedir)/media
nobase_resipmediainclude_HEADERS = \
        RTPPortManager.hxx

