
EXTRA_DIST = ca.pem
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj

LDADD = ../libreTurnClient.la
LDADD += ../../../rutil/librutil.la
LDADD += $(LIBSSL_LIBADD) @LIBPTHREAD_LIBADD@

# These tests all need to be run manually at the moment, so
# they are commented out (they still get compiled by `make check'):
#TESTS = TestClient
#TESTS += TestAsyncClient
#TESTs += TestRtpLoad

check_PROGRAMS = \
	TestClient \
	TestAsyncClient \
	TestRtpLoad

TestClient_SOURCES = TestClient.cxx
TestAsyncClient_SOURCES = TestAsyncClient.cxx
TestRtpLoad_SOURCES = TestRtpLoad.cxx


