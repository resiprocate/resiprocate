# $Id: Makefile,v 1.25 2002/12/20 22:32:32 alan Exp $

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
		testCountStream.cxx \
	        testDataPerformance.cxx \
	        testRandomHex.cxx \
	        testCoders.cxx \
		testThreadIf.cxx \
		testLogger.cxx

# Include architecture depenadant / cross compilation stuff.
# PLEASE LEAVE THIS ALONE
-include ../Makefile.opt


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
	CountStream.cxx \
	MD5Stream.cxx 

include $(BUILD)/Makefile.post

