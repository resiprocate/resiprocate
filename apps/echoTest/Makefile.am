
EXTRA_DIST = echoTest.config

bin_PROGRAMS = echoTest
echoTest_SOURCES = echoTest.cxx
echoTest_SOURCES += AppSubsystem.cxx
echoTest_SOURCES += EchoTestConfig.cxx

noinst_HEADERS = AppSubsystem.hxx
noinst_HEADERS += EchoTestConfig.hxx

echoTest_CXXFLAGS = $(GSTREAMERMM_1_0_CFLAGS)
echoTest_LDADD = $(GSTREAMERMM_1_0_LIBS) $(LDADD)
echoTest_LDADD += ../../resip/dum/libdum.la
echoTest_LDADD += ../../resip/stack/libresip.la
echoTest_LDADD += ../../rutil/librutil.la


