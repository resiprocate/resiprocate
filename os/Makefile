# $Id: Makefile,v 1.20 2002/11/23 22:06:27 fluffy Exp $

BUILD = ../../build

USE_SSL = 1

include $(BUILD)/Makefile.pre

USE_REPO=false

PACKAGES += SIP2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TARGET_LIBRARY = libutil2

TESTPROGRAMS =	testParseBuffer.cxx \
	        testData.cxx \
	        testDataStream.cxx \
	        testDataPerformance.cxx \
	        testRandomHex.cxx \
	        testCoders.cxx

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \
	Coders.cxx \
	Condition.cxx \
	Data.cxx \
	Lock.cxx \
	Log.cxx \
	Logger.cxx \
	Mutex.cxx \
	RWMutex.cxx \
	ParseBuffer.cxx \
	Random.cxx \
	Socket.cxx \
	Subsystem.cxx \
	ThreadIf.cxx \
	Timer.cxx \
	BaseException.cxx \
	vmd5.cxx \
	DataStream.cxx \

include $(BUILD)/Makefile.post

