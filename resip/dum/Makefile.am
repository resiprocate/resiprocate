# $Id$

EXTRA_DIST = Doxyfile
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj
EXTRA_DIST += doc

SUBDIRS = . test

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = libdum.la

libdum_la_LIBADD = ../stack/libresip.la
libdum_la_LIBADD += ../../rutil/librutil.la
libdum_la_LIBADD += @LIBSSL_LIBADD@ @LIBRADIUS_LIBADD@
libdum_la_LIBADD += @LIBSTL_LIBADD@
libdum_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libdum_la_SOURCES =   \
	AppDialog.cxx \
	AppDialogSet.cxx \
	AppDialogSetFactory.cxx \
	BaseCreator.cxx \
	BaseUsage.cxx \
	UserAuthInfo.cxx \
	BaseSubscription.cxx \
	ChallengeInfo.cxx \
	ClientAuthManager.cxx \
	ClientAuthExtension.cxx \
	ClientInviteSession.cxx \
	ClientOutOfDialogReq.cxx \
	ClientPagerMessage.cxx \
	ClientPublication.cxx \
	ClientRegistration.cxx \
	ClientSubscription.cxx \
	ContactInstanceRecord.cxx \
	DefaultServerReferHandler.cxx \
	DestroyUsage.cxx \
	Dialog.cxx \
	DialogEventInfo.cxx \
	DialogEventStateManager.cxx \
	DialogId.cxx \
	DialogSet.cxx \
	DialogSetId.cxx \
	DialogUsage.cxx \
	DialogUsageManager.cxx \
	DumProcessHandler.cxx \
	DumThread.cxx \
	DumTimeout.cxx \
	EncryptionRequest.cxx \
	HandleException.cxx \
	HandleManager.cxx \
	Handle.cxx \
	Handled.cxx \
	InMemoryRegistrationDatabase.cxx \
	InMemorySyncPubDb.cxx \
	InMemorySyncRegDb.cxx \
	InviteSession.cxx \
	InviteSessionCreator.cxx \
	InviteSessionHandler.cxx \
	MergedRequestKey.cxx \
	NonDialogUsage.cxx \
	OutOfDialogReqCreator.cxx \
	PagerMessageCreator.cxx \
	MasterProfile.cxx \
	UserProfile.cxx \
	Profile.cxx \
	PublicationCreator.cxx \
	RADIUSServerAuthManager.cxx \
	RedirectManager.cxx \
	RegistrationCreator.cxx \
	RegistrationHandler.cxx \
	ServerAuthManager.cxx \
	ServerInviteSession.cxx \
	ServerOutOfDialogReq.cxx \
	ServerPagerMessage.cxx \
	ServerPublication.cxx \
	ServerRegistration.cxx \
	ServerSubscription.cxx \
	SubscriptionHandler.cxx \
	SubscriptionCreator.cxx \
	SubscriptionState.cxx \
    TlsPeerAuthManager.cxx \
    WsCookieAuthManager.cxx \
	KeepAliveManager.cxx \
	KeepAliveTimeout.cxx \
	NetworkAssociation.cxx \
	DumDecrypted.cxx \
	CertMessage.cxx \
	DumFeatureChain.cxx \
	DumFeatureMessage.cxx \
	IdentityHandler.cxx \
	TargetCommand.cxx \
	DumFeature.cxx \
	OutgoingEvent.cxx \
	HttpProvider.cxx \
	HttpGetMessage.cxx \
	DumHelper.cxx \
	MergedRequestRemovalCommand.cxx

if USE_SSL
libdum_la_SOURCES += \
	ssl/EncryptionManager.cxx
endif


dumincludedir = $(includedir)/resip/dum
nobase_duminclude_HEADERS = AppDialog.hxx \
	AppDialogSetFactory.hxx \
	AppDialogSet.hxx \
	BaseCreator.hxx \
	BaseSubscription.hxx \
	BaseUsage.hxx \
	CertMessage.hxx \
	ChallengeInfo.hxx \
	ClientAuthExtension.hxx \
	ClientAuthManager.hxx \
	ClientInviteSession.hxx \
	ClientOutOfDialogReq.hxx \
	ClientPagerMessage.hxx \
	ClientPublication.hxx \
	ClientRegistration.hxx \
	ClientSubscriptionFunctor.hxx \
	ClientSubscription.hxx \
	ContactInstanceRecord.hxx \
	DefaultServerReferHandler.hxx \
	DestroyUsage.hxx \
	DialogEventHandler.hxx \
	DialogEventInfo.hxx \
	DialogEventStateManager.hxx \
	Dialog.hxx \
	DialogId.hxx \
	DialogSetHandler.hxx \
	DialogSet.hxx \
	DialogSetId.hxx \
	DialogUsage.hxx \
	DialogUsageManager.hxx \
	DumCommand.hxx \
	DumDecrypted.hxx \
	DumException.hxx \
	DumFeatureChain.hxx \
	DumFeature.hxx \
	DumFeatureMessage.hxx \
	DumHelper.hxx \
	DumProcessHandler.hxx \
	DumShutdownHandler.hxx \
	DumThread.hxx \
	DumTimeout.hxx \
	EncryptionRequest.hxx \
	EventDispatcher.hxx \
	ExternalMessageBase.hxx \
	ExternalMessageHandler.hxx \
	ExternalTimer.hxx \
	Handled.hxx \
	HandleException.hxx \
	Handle.hxx \
	HandleManager.hxx \
	Handles.hxx \
	HttpGetMessage.hxx \
	HttpProvider.hxx \
	IdentityHandler.hxx \
	InMemoryRegistrationDatabase.hxx \
	InMemorySyncPubDb.hxx \
	InMemorySyncRegDb.hxx \
	InviteDialogs.hxx \
	InviteSessionCreator.hxx \
	InviteSessionHandler.hxx \
	InviteSession.hxx \
	KeepAliveManager.hxx \
	KeepAliveTimeout.hxx \
	MasterProfile.hxx \
	MergedRequestKey.hxx \
	MergedRequestRemovalCommand.hxx \
	NetworkAssociation.hxx \
	NonDialogUsage.hxx \
	OutgoingEvent.hxx \
	OutOfDialogHandler.hxx \
	OutOfDialogReqCreator.hxx \
	PagerMessageCreator.hxx \
	PagerMessageHandler.hxx \
	Postable.hxx \
	Profile.hxx \
	PublicationCreator.hxx \
	PublicationHandler.hxx \
	PublicationPersistenceManager.hxx \
	RADIUSServerAuthManager.hxx \
	RedirectHandler.hxx \
	RedirectManager.hxx \
	RefCountedDestroyer.hxx \
	RegistrationCreator.hxx \
	RegistrationHandler.hxx \
	RegistrationPersistenceManager.hxx \
	RemoteCertStore.hxx \
	RequestValidationHandler.hxx \
	ServerAuthManager.hxx \
	ServerInviteSession.hxx \
	ServerOutOfDialogReq.hxx \
	ServerPagerMessage.hxx \
	ServerPublication.hxx \
	ServerRegistration.hxx \
	ServerSubscriptionFunctor.hxx \
	ServerSubscription.hxx \
	ssl/EncryptionManager.hxx \
	SubscriptionCreator.hxx \
	SubscriptionHandler.hxx \
	SubscriptionPersistenceManager.hxx \
	SubscriptionState.hxx \
	TargetCommand.hxx \
        TlsPeerAuthManager.hxx \
        WsCookieAuthManager.hxx \
	UsageUseException.hxx \
	UserAuthInfo.hxx \
	UserProfile.hxx

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
