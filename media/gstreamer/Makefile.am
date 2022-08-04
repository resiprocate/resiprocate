
EXTRA_DIST = README.txt

if USE_GSTREAMER

lib_LTLIBRARIES = libgstreamerutils.la

libgstreamerutils_la_CXXFLAGS = $(GSTREAMERMM_1_0_CFLAGS)
libgstreamerutils_la_LIBADD = $(GSTREAMERMM_1_0_LIBS) -lgstwebrtc-1.0
libgstreamerutils_la_LIBADD += ../../resip/stack/libresip.la
libgstreamerutils_la_LIBADD += ../../rutil/librutil.la
libgstreamerutils_la_LIBADD += @LIBSTL_LIBADD@
libgstreamerutils_la_LIBADD += @LIBPTHREAD_LIBADD@
libgstreamerutils_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libgstreamerutils_la_SOURCES = \
	GStreamerUtils.cxx

gstreamerutilsincludedir = $(includedir)/media/gstreamer
nobase_gstreamerutilsinclude_HEADERS = \
	GStreamerUtils.hxx

endif

