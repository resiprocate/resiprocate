# $Id: Makefile,v 1.15 2002/11/04 20:50:58 alan Exp $

BUILD = ../../build

include $(BUILD)/Makefile.pre

USE_REPO=false

PACKAGES += SIP2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TARGET_LIBRARY = libutil2

TESTPROGRAMS = testParseBuffer.cxx testData.cxx testDataPerformance.cxx  testCoders.cxx

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \
	Coders.cxx \
	Condition.cxx \
	Data.cxx \
	Fifo.cxx \
	Lock.cxx \
	Log.cxx \
	Logger.cxx \
	Mutex.cxx \
	ParseBuffer.cxx \
	RandomHex.cxx \
	Socket.cxx \
	Subsystem.cxx \
	ThreadIf.cxx \
	Timer.cxx \
	VException.cxx \
	vmd5.cxx \
	DataStream.cxx \

include $(BUILD)/Makefile.post

