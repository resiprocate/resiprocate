# $Id$

AM_CXXFLAGS = -DUSE_ARES -DUSE_SSL

AM_LDFLAGS = -lssl

sbin_PROGRAMS = repro
repro_SOURCES = repro.cxx
repro_LDADD = librepro.la
repro_LDADD += ../resip/dum/libdum.la
repro_LDADD += ../resip/stack/libresip.la ../rutil/librutil.la
repro_LDADD += ../contrib/ares/libares.a
#repro_LDADD += -L../contrib/ares -lares
repro_LDADD += -lssl -lpthread -ldb_cxx-4.8

lib_LTLIBRARIES = librepro.la

librepro_la_LDFLAGS = -version-info 0:0:0 -release 1 -export-dynamic

librepro_la_SOURCES = \
	RouteStore.cxx \
	UserStore.cxx \
	ConfigStore.cxx \
	AclStore.cxx \
	Store.cxx \
	AbstractDb.cxx \
	BerkeleyDb.cxx \
	\
	CommandLineParser.cxx \
	ReproVersion.cxx \
	HttpBase.cxx \
	HttpConnection.cxx \
	WebAdmin.cxx \
	WebAdminThread.cxx \
	\
	Proxy.cxx \
	Registrar.cxx \
	RegSyncClient.cxx \
	RegSyncServer.cxx \
	RegSyncServerThread.cxx \
	ReproServerAuthManager.cxx \
	RequestContext.cxx \
	ResponseContext.cxx \
	RRDecorator.cxx \
        Processor.cxx \
        ProcessorChain.cxx \
	Target.cxx \
	WorkerThread.cxx \
	XmlRpcConnection.cxx \
	XmlRpcServerBase.cxx \
	Dispatcher.cxx \
	OutboundTarget.cxx \
	QValueTarget.cxx \
	\
        monkeys/DigestAuthenticator.cxx \
	monkeys/StrictRouteFixup.cxx \
	monkeys/AmIResponsible.cxx \
	monkeys/IsTrustedNode.cxx \
	monkeys/ConstantLocationMonkey.cxx \
	monkeys/LocationServer.cxx \
	monkeys/OutboundTargetHandler.cxx \
	monkeys/RecursiveRedirect.cxx \
	monkeys/SimpleStaticRoute.cxx \
	monkeys/StaticRoute.cxx \
	monkeys/QValueTargetHandler.cxx \
	monkeys/SimpleTargetHandler.cxx \
	\
	MySqlDb.cxx \
	stateAgents/CertServer.cxx \
	stateAgents/CertPublicationHandler.cxx \
	stateAgents/CertSubscriptionHandler.cxx \
	stateAgents/PrivateKeyPublicationHandler.cxx \
	stateAgents/PrivateKeySubscriptionHandler.cxx

# reproInfo.hxx is created manually be other build systems
# but here we just delegate to the autoconf generated config.h
reproInfo.hxx:
	@echo "#include \"config.h\"" >  reproInfo.hxx
	cat reproInfo.hxx

ReproVersion.cxx: reproInfo.hxx

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
