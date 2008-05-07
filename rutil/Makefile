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
	ParseException.cxx \
	Poll.cxx \
	RWMutex.cxx \
	Random.cxx \
	RecursiveMutex.cxx \
	SHA1Stream.cxx \
	Socket.cxx \
	Subsystem.cxx \
	SysLogBuf.cxx \
	SysLogStream.cxx \
	ThreadIf.cxx \
	Timer.cxx \
	TransportType.cxx \
	vmd5.cxx \
	\
	dns/AresDns.cxx \
	dns/DnsAAAARecord.cxx \
	dns/DnsCnameRecord.cxx \
	dns/DnsHostRecord.cxx \
	dns/DnsNaptrRecord.cxx \
	dns/DnsResourceRecord.cxx \
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

ifeq ($(OSTYPE),MinGW)
    SRC += WinCompat.cxx
endif

include $(BUILD)/Makefile.post

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
