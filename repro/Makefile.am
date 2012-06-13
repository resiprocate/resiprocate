# $Id$

EXTRA_DIST = *.html
EXTRA_DIST += doc
EXTRA_DIST += Doxyfile
EXTRA_DIST += etc
EXTRA_DIST += php
EXTRA_DIST += README_MySQL.txt
EXTRA_DIST += repro_10_0.vcxproj repro_10_0.vcxproj.filters
EXTRA_DIST += reprolib_10_0.vcxproj reprolib_10_0.vcxproj.filters
EXTRA_DIST += *.vcproj
#EXTRA_DIST += *.db
EXTRA_DIST += repro.spec
EXTRA_DIST += VERSION
EXTRA_DIST += webadmin
EXTRA_DIST += WinSetup WinSetupx64
EXTRA_DIST += repro.config
EXTRA_DIST += create_mysql_reprodb.sql

SUBDIRS = . test reprocmd accountingconsumers

#AM_CXXFLAGS = -DUSE_ARES

sbin_PROGRAMS = repro
repro_SOURCES = repro.cxx
repro_LDADD = librepro.la
repro_LDADD += ../resip/dum/libdum.la
repro_LDADD += ../resip/stack/libresip.la ../rutil/librutil.la
#repro_LDADD += ../contrib/ares/libares.a
#repro_LDADD += -L../contrib/ares -lares
repro_LDADD += @LIBSSL_LIBADD@ -lpthread

lib_LTLIBRARIES = librepro.la

librepro_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic
librepro_la_LIBADD = -ldb_cxx

librepro_la_SOURCES = \
	RouteStore.cxx \
	UserStore.cxx \
	ConfigStore.cxx \
	AclStore.cxx \
    StaticRegStore.cxx \
	FilterStore.cxx \
	SiloStore.cxx \
	Store.cxx \
	AbstractDb.cxx \
	BerkeleyDb.cxx \
	\
	CommandServer.cxx \
	CommandServerThread.cxx \
	ProxyConfig.cxx \
	ReproVersion.cxx \
	HttpBase.cxx \
	HttpConnection.cxx \
	WebAdmin.cxx \
	WebAdminThread.cxx \
	\
	AccountingCollector.cxx \
	Proxy.cxx \
	Registrar.cxx \
	RegSyncClient.cxx \
	RegSyncServer.cxx \
	RegSyncServerThread.cxx \
	ReproRunner.cxx \
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
	PersistentMessageQueue.cxx \
	QValueTarget.cxx \
	\
        monkeys/CertificateAuthenticator.cxx \
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
	monkeys/GeoProximityTargetSorter.cxx \
	monkeys/RequestFilter.cxx \
	monkeys/MessageSilo.cxx

reproincludedir = $(includedir)/repro
nobase_reproinclude_HEADERS = AbstractDb.hxx \
	AccountingCollector.hxx \
	Ack200DoneMessage.hxx \
	AclStore.hxx \
        AsyncProcessor.hxx \
        AsyncProcessorMessage.hxx \
        AsyncProcessorWorker.hxx \
	BerkeleyDb.hxx \
	CommandServer.hxx \
	CommandServerThread.hxx \
	ConfigStore.hxx \
	Dispatcher.hxx \
	FilterStore.hxx \
	ForkControlMessage.hxx \
	HttpBase.hxx \
	HttpConnection.hxx \
	monkeys/AmIResponsible.hxx \
        monkeys/CertificateAuthenticator.hxx \
	monkeys/ConstantLocationMonkey.hxx \
	monkeys/DigestAuthenticator.hxx \
	monkeys/GruuMonkey.hxx \
	monkeys/IsTrustedNode.hxx \
	monkeys/LocationServer.hxx \
	monkeys/OutboundTargetHandler.hxx \
	monkeys/QValueTargetHandler.hxx \
	monkeys/RecursiveRedirect.hxx \
	monkeys/SimpleStaticRoute.hxx \
	monkeys/SimpleTargetHandler.hxx \
	monkeys/StaticRoute.hxx \
	monkeys/StrictRouteFixup.hxx \
	monkeys/GeoProximityTargetSorter.hxx \
	monkeys/RequestFilter.hxx \
	monkeys/MessageSilo.hxx \
	MySqlDb.hxx \
	OutboundTarget.hxx \
	PersistentMessageQueue.hxx \
	ProcessorChain.hxx \
	Processor.hxx \
	ProcessorMessage.hxx \
	Proxy.hxx \
	ProxyConfig.hxx \
	QValueTarget.hxx \
	Registrar.hxx \
	RegSyncClient.hxx \
	RegSyncServer.hxx \
	RegSyncServerThread.hxx \
	reproInfo.hxx \
	ReproServerAuthManager.hxx \
	ReproRunner.hxx \
	ReproVersion.hxx \
	RequestContext.hxx \
	ResponseContext.hxx \
	RouteStore.hxx \
	RRDecorator.hxx \
	SiloStore.hxx \
	stateAgents/CertPublicationHandler.hxx \
	stateAgents/CertServer.hxx \
	stateAgents/CertSubscriptionHandler.hxx \
	stateAgents/PrivateKeyPublicationHandler.hxx \
	stateAgents/PrivateKeySubscriptionHandler.hxx \
	StaticRegStore.hxx \
	Store.hxx \
	Target.hxx \
	TimerCMessage.hxx \
	UserAuthGrabber.hxx \
	UserInfoMessage.hxx \
	UserStore.hxx \
	WebAdmin.hxx \
	WebAdminThread.hxx \
	Worker.hxx \
	WorkerThread.hxx \
	XmlRpcConnection.hxx \
	XmlRpcServerBase.hxx

if USE_SSL
librepro_la_SOURCES += \
	stateAgents/CertServer.cxx \
	stateAgents/CertPublicationHandler.cxx \
	stateAgents/CertSubscriptionHandler.cxx \
	stateAgents/PrivateKeyPublicationHandler.cxx \
	stateAgents/PrivateKeySubscriptionHandler.cxx
endif

if USE_MYSQL
librepro_la_SOURCES += MySqlDb.cxx
librepro_la_LIBADD += @LIBMYSQL_LIBADD@
endif

if USE_MAXMIND_GEOIP
librepro_la_LIBADD += @LIBGEOIP_LIBADD@
endif

# reproInfo.hxx is created manually be other build systems
# but here we just delegate to the autoconf generated config.h
reproInfo.hxx:
	@echo "#include \"config.h\"" >  reproInfo.hxx
	cat reproInfo.hxx

ReproVersion.cxx: reproInfo.hxx

dist_man_MANS = doc/repro.8
dist_man_MANS += doc/reprocmd.8

sysconf_DATA = etc/repro.conf

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
