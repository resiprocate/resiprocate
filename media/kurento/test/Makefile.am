
AM_CXXFLAGS = -I $(top_srcdir)

LDADD = ../libkurentoclient.la
LDADD += ../../../rutil/librutil.la
LDADD += $(LIBSSL_LIBADD) @LIBSTL_LIBADD@ @LIBPTHREAD_LIBADD@

# we don't run this automatically because people will not always
# have a local Kurento server available when running the unit tests
# for the rest of the stack
#TESTS = \
#	testKurentoClient

check_PROGRAMS = \
	testKurentoClient

testKurentoClient_SOURCES = testKurentoClient.cxx

