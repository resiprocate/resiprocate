# $Id: Makefile,v 1.26 2003/03/24 22:42:27 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE OPENSSL PTHREAD 
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

