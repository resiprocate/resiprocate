# $Id$

if USE_ARES
SUBDIRS = dns/ares
else
SUBDIRS = 
endif

SUBDIRS += . test

EXTRA_DIST = fixupGperf
EXTRA_DIST += mainpage.doc
EXTRA_DIST += *.vcxproj
EXTRA_DIST += WinCompat.cxx
EXTRA_DIST += msvc/*.h
EXTRA_DIST += dns/LocalDns.cxx

librutil_la_CXXFLAGS = -I $(top_srcdir)
if USE_ARES
librutil_la_CXXFLAGS += -I $(top_srcdir)/rutil/dns/ares
endif

lib_LTLIBRARIES = librutil.la

librutil_la_LIBADD = $(LIBSSL_LIBADD)
librutil_la_LIBADD += $(LIBRADIUS_LIBADD)
librutil_la_LIBADD += $(LIBARES_LIBADD)
librutil_la_LIBADD += @LIBSTL_LIBADD@
librutil_la_LIBADD += $(DEPS_FMT_LIBS)
librutil_la_LIBADD += @LIBLOG_LIBADD@
#librutil_la_LIBADD += -lrt
librutil_la_LIBADD += @LIBPTHREAD_LIBADD@
librutil_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

librutil_la_SOURCES = \
	AbstractFifo.cxx \
	AndroidLogger.cxx \
	BaseException.cxx \
	Coders.cxx \
	ConfigParse.cxx \
	CountStream.cxx \
        Crc32.cxx \
	ServerProcess.cxx \
	Data.cxx \
	DataStream.cxx \
	DnsUtil.cxx \
	FileSystem.cxx \
	GeneralCongestionManager.cxx \
	GenericIPAddress.cxx \
	HeapInstanceCounter.cxx \
	KeyValueStore.cxx \
	Lock.cxx \
	Log.cxx \
	MD5Stream.cxx \
	NetNs.cxx \
	ParseBuffer.cxx \
	ParseException.cxx \
	Poll.cxx \
	PoolBase.cxx \
	FdPoll.cxx \
	RADIUSDigestAuthenticator.cxx \
	RWMutex.cxx \
	Random.cxx \
	resipfaststreams.cxx \
	SelectInterruptor.cxx \
	Sha1.cxx \
	Socket.cxx \
	Subsystem.cxx \
	SysLogBuf.cxx \
	SysLogStream.cxx \
	ThreadIf.cxx \
	Time.cxx \
	Timer.cxx \
	TransportType.cxx \
	vmd5.cxx \
	XMLCursor.cxx \
	\
	dns/AresDns.cxx \
	dns/DnsCnameRecord.cxx \
	dns/DnsAAAARecord.cxx \
	dns/DnsHostRecord.cxx \
	dns/DnsNaptrRecord.cxx \
	dns/DnsResourceRecord.cxx \
	dns/DnsThread.hxx \
	dns/DnsSrvRecord.cxx \
	dns/DnsStub.cxx \
	dns/DnsThread.cxx \
	dns/ExternalDnsFactory.cxx \
	dns/RRCache.cxx \
	dns/RRList.cxx \
	dns/RRVip.cxx \
	dns/QueryTypes.cxx \
	dns/RROverlay.cxx \
	\
	stun/Stun.cxx \
	stun/Udp.cxx \
	\
	hep/HepAgent.cxx \
	hep/ResipHep.cxx

if USE_SSL
librutil_la_SOURCES += \
	ssl/OpenSSLInit.cxx \
	ssl/SHA1Stream.cxx
endif

if BUILD_GSTREAMER
librutil_la_SOURCES += \
	GStreamerUtils.cxx
librutil_la_CXXFLAGS += $(GSTREAMERMM_1_0_CFLAGS)
librutil_la_LIBADD += $(GSTREAMERMM_1_0_LIBS)
# $(LDADD)
endif

rutilincludedir = $(includedir)/rutil
nobase_rutilinclude_HEADERS = compat.hxx \
	ResipAssert.h \
	FileSystem.hxx \
	HashMap.hxx \
	wince/WceCompat.hxx \
	SysLogStream.hxx \
	AsyncID.hxx \
	AsyncBool.hxx \
	ConfigParse.hxx \
	CongestionManager.hxx \
	GeneralCongestionManager.hxx \
	GStreamerUtils.hxx \
	HeapInstanceCounter.hxx \
	KeyValueStore.hxx \
	FdSetIOObserver.hxx \
	Fifo.hxx \
	CircularBuffer.hxx \
	FiniteFifo.hxx \
	ParseBuffer.hxx \
	Log.hxx \
	ThreadIf.hxx \
	WinLeakCheck.hxx \
	Random.hxx \
	AsyncProcessHandler.hxx \
	RADIUSDigestAuthenticator.hxx \
	Repository.hxx \
	RWMutex.hxx \
	Subsystem.hxx \
	Logger.hxx \
	MediaConstants.hxx \
	MD5Stream.hxx \
	DnsUtil.hxx \
	Timer.hxx \
	DigestStream.hxx \
	TransportType.hxx \
	resipfaststreams.hxx \
	Coders.hxx \
	Sha1.hxx \
	SelectInterruptor.hxx \
	Socket.hxx \
	dns/ExternalDnsFactory.hxx \
	dns/DnsStub.hxx \
	dns/DnsHostRecord.hxx \
	dns/QueryTypes.hxx \
	dns/RROverlay.hxx \
	dns/AresCompat.hxx \
	dns/ExternalDns.hxx \
	dns/DnsNaptrRecord.hxx \
	dns/RRList.hxx \
	dns/LocalDns.hxx \
	dns/RRFactory.hxx \
	dns/DnsSrvRecord.hxx \
	dns/DnsCnameRecord.hxx \
	dns/AresDns.hxx \
	dns/RRCache.hxx \
	dns/DnsHandler.hxx \
	dns/RRVip.hxx \
	dns/DnsAAAARecord.hxx \
	dns/DnsResourceRecord.hxx \
	Condition.hxx \
	WinCompat.hxx \
	vthread.hxx \
	ServerProcess.hxx \
	Data.hxx \
	Lock.hxx \
	TimeLimitFifo.hxx \
	Mutex.hxx \
	NetNs.hxx \
	GenericTimerQueue.hxx \
	IntrusiveListElement.hxx \
	ssl/SHA1Stream.hxx \
	ssl/OpenSSLInit.hxx \
	CountStream.hxx \
        Crc32.hxx \
	vmd5.hxx \
	XMLCursor.hxx \
	PoolBase.hxx \
	FdPoll.hxx \
	Time.hxx \
	stun/Udp.hxx \
	stun/Stun.hxx \
	SysLogBuf.hxx \
	Inserter.hxx \
	DataStream.hxx \
	GenericIPAddress.hxx \
	AbstractFifo.hxx \
	AndroidLogger.hxx \
	ParseException.hxx \
	BaseException.hxx \
	DataException.hxx \
	Poll.hxx \
	StlPoolAllocator.hxx \
	ProducerFifoBuffer.hxx \
	DinkyPool.hxx \
	ConsumerFifoBuffer.hxx \
	hep/HepAgent.hxx \
	hep/ResipHep.hxx

Log.cxx:	Repository.hxx

Repository.hxx:	.ALWAYS_REBUILD
	echo "// Auto generated" > $@.tmp
	echo "#ifndef __Repository_hxx" >> $@.tmp
	echo "#define __Repository_hxx" >> $@.tmp
	echo "#define RESIPROCATE_GIT_ID \"`git describe --match="" --always --abbrev=40 --dirty`\"" >> $@.tmp
	echo "#define RESIPROCATE_BRANCH_NAME \"`git rev-parse --abbrev-ref HEAD`\"" >> $@.tmp
	echo "#endif" >> $@.tmp
	diff -q $@ $@.tmp || mv $@.tmp $@

.ALWAYS_REBUILD:


##############################################################################
# 
# The Vovida Software License, Version 1.0 
# Copyright (c) 2000-2007 Vovida Networks, Inc.  All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 
# 3. The names "VOCAL", "Vovida Open Communication Application Library",
#    and "Vovida Open Communication Application Library (VOCAL)" must
#    not be used to endorse or promote products derived from this
#    software without prior written permission. For written
#    permission, please contact vocal@vovida.org.
# 
# 4. Products derived from this software may not be called "VOCAL", nor
#    may "VOCAL" appear in their name, without prior written
#    permission of Vovida Networks, Inc.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
# NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
# IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
# 
# ====================================================================
# 
# This software consists of voluntary contributions made by Vovida
# Networks, Inc. and many individuals on behalf of Vovida Networks,
# Inc.  For more information on Vovida Networks, Inc., please see
# <http://www.vovida.org/>.
# 
##############################################################################
