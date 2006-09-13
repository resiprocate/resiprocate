# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += ARES PTHREAD OPENSSL

CXXFLAGS += -I..
CODE_SUBDIRS = dns stun
TARGET_LIBRARY = librutil
TESTPROGRAMS =

SRC = \
	AbstractFifo.cxx \
	BaseException.cxx \
	Coders.cxx \
	Condition.cxx \
	CountStream.cxx \
	Data.cxx \
	DataStream.cxx \
	DnsUtil.cxx \
	FileSystem.cxx \
	HeapInstanceCounter.cxx \
	Lock.cxx \
	Log.cxx \
	Logger.cxx \
	MD5Stream.cxx \
	Mutex.cxx \
	OpenSSLInit.cxx \
	ParseBuffer.cxx \
	Poll.cxx \
	RWMutex.cxx \
	Random.cxx \
	RecursiveMutex.cxx \
	SelectInterruptor.cxx \
	SHA1Stream.cxx \
	Socket.cxx \
	Subsystem.cxx \
	SysLogBuf.cxx \
	SysLogStream.cxx \
	ThreadIf.cxx \
	Timer.cxx \
	vmd5.cxx \
	\
	dns/AresDns.cxx \
	dns/DnsAAAARecord.cxx \
	dns/DnsCnameRecord.cxx \
	dns/DnsHostRecord.cxx \
	dns/DnsNaptrRecord.cxx \
	dns/DnsSrvRecord.cxx \
	dns/DnsStub.cxx \
	dns/ExternalDnsFactory.cxx \
	dns/RRCache.cxx \
	dns/RRList.cxx \
	dns/RRVip.cxx \
	dns/QueryTypes.cxx \
	dns/RROverlay.cxx \
	\
	stun/Stun.cxx \
	stun/Udp.cxx


include $(BUILD)/Makefile.post
