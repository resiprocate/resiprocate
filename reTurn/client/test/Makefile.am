
EXTRA_DIST = ca.pem
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj

LDADD = ../libreTurnClient.la
LDADD += ../../../rutil/librutil.la
LDADD += $(LIBSSL_LIBADD) -lpthread

TESTS = \
        TestClient \
        TestAsyncClient \
        TestRtpLoad

check_PROGRAMS = \
	TestClient \
	TestAsyncClient \
	TestRtpLoad

TestClient_SOURCES = TestClient.cxx
TestAsyncClient_SOURCES = TestAsyncClient.cxx
TestRtpLoad_SOURCES = TestRtpLoad.cxx


