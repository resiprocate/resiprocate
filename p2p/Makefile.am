# $Id$

#AM_CXXFLAGS = -DUSE_ARES 
AM_CXXFLAGS = -I $(top_srcdir) -I $(top_srcdir)/p2p/s2c

lib_LTLIBRARIES = libp2p.la

libp2p_la_LIBADD = s2c/s2c/libs2c.la
libp2p_la_LIBADD += ../rutil/librutil.la
libp2p_la_LIBADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@
libp2p_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libp2p_la_SOURCES =   \
        ChordTopology.cxx \
        ChordUpdate.cxx \
        Connect.cxx \
        ConnectBase.cxx \
        DestinationId.cxx \
        Dispatcher.cxx \
        Find.cxx \
        FlowId.cxx \
        ForwardingLayer.cxx \
        Join.cxx \
        Leave.cxx \
        Message.cxx \
        MessageHelper.cxx \
        MessageStructsGen.cxx \
        NodeId.cxx \
        P2PStack.cxx \
        P2PSubsystem.cxx \
        ResourceId.cxx \
        SelectTransporter.cxx \
        Signable.cxx \
        SignatureContext.cxx \
        TestMake.cxx \
        TopologyAPI.cxx \
        Transporter.cxx \
        TransporterMessage.cxx \
        Update.cxx \
        BatchMessages.cxx

noinst_HEADERS = AbstractValue.hxx \
	ArrayValue.hxx \
	BatchMessages.hxx \
	Candidate.hxx \
	CertDoneEvent.hxx \
	ChordTopology.hxx \
	ChordUpdate.hxx \
	ConnectBase.hxx \
	Connect.hxx \
	DataSpecifier.hxx \
	DestinationId.hxx \
	DictionaryValue.hxx \
	Dispatcher.hxx \
	EventConsumerBase.hxx \
	EventConsumer.hxx \
	Event.hxx \
	EventWrapper.hxx \
	FetchAns.hxx \
	FetchKind.hxx \
	FetchReq.hxx \
	Find.hxx \
	FlowId.hxx \
	ForwardingLayer.hxx \
	Join.hxx \
	Leave.hxx \
	MessageHelper.hxx \
	Message.hxx \
	MessageStructsGen.hxx \
	NodeId.hxx \
	p2p.hxx \
	P2PStack.hxx \
	P2PSubsystem.hxx \
	Postable.hxx \
	Profile.hxx \
	ResourceId.hxx \
	SelectTransporter.hxx \
	Signable.hxx \
	SignatureContext.hxx \
	SingleValue.hxx \
	StoreAns.hxx \
	StoreReq.hxx \
	StoreSet.hxx \
	tmp/ForwardingGen.hxx \
	tmp/fwding.hxx \
	TopologyAPI.hxx \
	TransactionController.hxx \
	TransactionState.hxx \
	TransApi.hxx \
	Transporter.hxx \
	TransporterMessage.hxx \
	Update.hxx \
	UserName.hxx


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
