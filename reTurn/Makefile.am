# $Id$

EXTRA_DIST = dh2048.pem server.pem
EXTRA_DIST += pkg
EXTRA_DIST += README.txt
EXTRA_DIST += *.sln
EXTRA_DIST += *.vcproj
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += reTurnServer.config
EXTRA_DIST += users.txt

dist_man_MANS = reTurnServer.8

dist_returnpkglib_SCRIPTS = psql-user-extract

SUBDIRS = .
SUBDIRS += client
SUBDIRS += test

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

sbin_PROGRAMS = reTurnServer
reTurnServer_SOURCES = reTurnServer.cxx \
	AsyncSocketBase.cxx \
        AsyncUdpSocketBase.cxx \
        AsyncTcpSocketBase.cxx \
        AsyncTlsSocketBase.cxx \
        ChannelManager.cxx \
        ConnectionManager.cxx \
        DataBuffer.cxx \
        RemotePeer.cxx \
        RequestHandler.cxx \
        ReTurnConfig.cxx \
        ReTurnSubsystem.cxx \
        StunAuth.cxx \
        StunMessage.cxx \
        StunTuple.cxx \
        TcpConnection.cxx \
        TcpServer.cxx \
        TlsConnection.cxx \
        TlsServer.cxx \
        TurnAllocation.cxx \
        TurnAllocationKey.cxx \
		TurnAllocationManager.cxx \
        TurnManager.cxx \
        TurnPermission.cxx \
        UdpRelayServer.cxx \
        UdpServer.cxx \
	UserAuthData.cxx

reTurnincludedir = $(includedir)/reTurn
nobase_reTurninclude_HEADERS = AsyncSocketBaseHandler.hxx \
	AsyncSocketBase.hxx \
	AsyncTcpSocketBase.hxx \
	AsyncTlsSocketBase.hxx \
	AsyncUdpSocketBase.hxx \
	ChannelManager.hxx \
	ConnectionManager.hxx \
	DataBuffer.hxx \
	RemotePeer.hxx \
	RequestHandler.hxx \
	ReTurnConfig.hxx \
	ReTurnSubsystem.hxx \
	StunAuth.hxx \
	StunMessage.hxx \
	StunTuple.hxx \
	TcpConnection.hxx \
	TcpServer.hxx \
	TlsConnection.hxx \
	TlsServer.hxx \
	TurnAllocation.hxx \
	TurnAllocationManager.hxx \
	TurnAllocationKey.hxx \
	TurnManager.hxx \
	TurnPermission.hxx \
	UdpRelayServer.hxx \
	UdpServer.hxx \
	UserAuthData.hxx \
	reTurnServer.hxx

#reTurnServer_LDADD = librepro.la
reTurnServer_LDADD = ../rutil/librutil.la
#reTurnServer_LDADD += ../contrib/ares/libares.a
#reTurnServer_LDADD += -L../contrib/ares -lares
reTurnServer_LDADD += @LIBSTL_LIBADD@
reTurnServer_LDADD += @LIBSSL_LIBADD@ -lcrypto @LIBPTHREAD_LIBADD@

#lib_LTLIBRARIES = librepro.la

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
