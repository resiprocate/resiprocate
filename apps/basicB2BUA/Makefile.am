
EXTRA_DIST = sample-config.xml

bin_PROGRAMS = basicB2BUA
basicB2BUA_LDADD = ../../b2bua/libb2bua.la
basicB2BUA_LDADD += ../../resip/dum/libdum.la
basicB2BUA_LDADD += ../../resip/stack/libresip.la
basicB2BUA_LDADD += ../../rutil/librutil.la
basicB2BUA_LDADD += -lxerces-c -lpcrecpp
basicB2BUA_LDADD += @LIBSSL_LIBADD@ -lpthread

basicB2BUA_SOURCES = \
	Context.cxx \
	ContextRule.cxx \
	MyServerAuthManager.cxx \
	Peer.cxx \
	BasicB2BUA.cxx \
	BasicConfiguration.cxx \
	BasicManager.cxx

noinst_HEADERS = BasicB2BUA.hxx \
	BasicConfiguration.hxx \
	BasicManager.hxx \
	Context.hxx \
	ContextRule.hxx \
	MyServerAuthManager.hxx \
	Peer.hxx \
	XMLHelpers.hxx

