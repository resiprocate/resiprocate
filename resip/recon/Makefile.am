# $Id$

EXTRA_DIST = readme.txt
EXTRA_DIST += readme_Kurento.txt
EXTRA_DIST += readme_Gstreamer.md
EXTRA_DIST += *.vcxproj
EXTRA_DIST += doc

SUBDIRS = .
SUBDIRS += test
SUBDIRS += MOHParkServer

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -DSIPX_NO_RECORD

lib_LTLIBRARIES = librecon.la
librecon_la_CXXFLAGS = $(AM_CXXFLAGS)
librecon_la_LIBADD = ../../reflow/libreflow.la
librecon_la_LIBADD += ../../reTurn/client/libreTurnClient.la
librecon_la_LIBADD += ../dum/libdum.la
librecon_la_LIBADD += ../stack/libresip.la
librecon_la_LIBADD += ../../rutil/librutil.la
librecon_la_LIBADD += ../../media/libresipmedia.la
if USE_SIPXTAPI
librecon_la_LIBADD += -lsipXmediaProcessing
librecon_la_LIBADD += -lsipXmedia
librecon_la_LIBADD += -lsipXsdp
librecon_la_LIBADD += -lsipXport
endif
if USE_KURENTO
librecon_la_LIBADD += ../../media/kurento/libkurentoclient.la
endif
if USE_SRTP1
librecon_la_LIBADD += @LIBSSL_LIBADD@ -lsrtp
else
librecon_la_LIBADD += @LIBSSL_LIBADD@ -lsrtp2
endif
librecon_la_LIBADD += @LIBPTHREAD_LIBADD@
librecon_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

librecon_la_SOURCES = BridgeMixer.cxx \
        Conversation.cxx \
        ConversationManager.cxx \
        ConversationParticipantAssignment.cxx \
        ConversationProfile.cxx \
        DefaultDialogSet.cxx \
        DtmfEvent.cxx \
        IMParticipantBase.cxx \
        LocalParticipant.cxx \
        MediaEvent.cxx \
        MediaStackAdapter.cxx \
        MediaResourceParticipant.cxx \
        MediaResourceCache.cxx \
        MediaStreamEvent.cxx \
        Participant.cxx \
        ReconSubsystem.cxx \
        RelatedConversationSet.cxx \
        RemoteParticipant.cxx \
        RemoteParticipantDialogSet.cxx \
        RemoteIMPagerParticipant.cxx \
        RemoteIMSessionParticipant.cxx \
        RemoteIMSessionParticipantDialogSet.cxx \
        UserAgent.cxx \
        UserAgentClientSubscription.cxx \
        UserAgentClientPublication.cxx \
        UserAgentDialogSetFactory.cxx \
        UserAgentMasterProfile.cxx \
        UserAgentRegistration.cxx \
        UserAgentServerAuthManager.cxx

if USE_SIPXTAPI
librecon_la_SOURCES += FlowManagerSipXSocket.cxx \
        SipXMediaStackAdapter.cxx \
        SipXMediaInterface.cxx \
        SipXConversation.cxx \
        SipXBridgeMixer.cxx \
        SipXParticipant.cxx \
        SipXLocalParticipant.cxx \
        SipXMediaResourceParticipant.cxx \
        SipXRemoteParticipant.cxx \
        SipXRemoteParticipantDialogSet.cxx \
        SipXHelper.cxx \
        sdp/Sdp.cxx \
        sdp/SdpCandidate.cxx \
        sdp/SdpCandidatePair.cxx \
        sdp/SdpCodec.cxx \
        sdp/SdpHelperResip.cxx \
        sdp/SdpMediaLine.cxx
endif

if USE_KURENTO
librecon_la_SOURCES += KurentoBridgeMixer.cxx \
        KurentoConversation.cxx \
        KurentoMediaStackAdapter.cxx \
        KurentoParticipant.cxx \
        KurentoRemoteParticipant.cxx \
        KurentoRemoteParticipantDialogSet.cxx
endif

if USE_GSTREAMER
librecon_la_CXXFLAGS += $(GSTREAMERMM_1_0_CFLAGS)
librecon_la_LIBADD += ../../media/gstreamer/libgstreamerutils.la
librecon_la_LIBADD += $(GSTREAMERMM_1_0_LIBS) -lgstwebrtc-1.0
librecon_la_SOURCES += GstBridgeMixer.cxx \
        GstConversation.cxx \
        GstMediaStackAdapter.cxx \
        GstParticipant.cxx \
        GstRemoteParticipant.cxx \
        GstRemoteParticipantDialogSet.cxx
endif

if USE_LIBWEBRTC
librecon_la_SOURCES += LibWebRTCBridgeMixer.cxx \
        LibWebRTCConversation.cxx \
        LibWebRTCMediaStackAdapter.cxx \
        LibWebRTCParticipant.cxx \
        LibWebRTCRemoteParticipant.cxx \
        LibWebRTCRemoteParticipantDialogSet.cxx
endif

reconincludedir = $(includedir)/recon
nobase_reconinclude_HEADERS = ReconSubsystem.hxx \
        LocalParticipant.hxx \
        MediaResourceCache.hxx \
        RelatedConversationSet.hxx \
        UserAgentServerAuthManager.hxx \
        ConversationManagerCmds.hxx \
        IMParticipantBase.hxx \
        UserAgentMasterProfile.hxx \
        RemoteParticipantDialogSet.hxx \
        SipXHelper.hxx \
        Conversation.hxx \
        UserAgentDialogSetFactory.hxx \
        UserAgentCmds.hxx \
        UserAgentRegistration.hxx \
        MediaEvent.hxx \
        MediaStackAdapter.hxx \
        DefaultDialogSet.hxx \
        Participant.hxx \
        UserAgentClientSubscription.hxx \
        UserAgentClientPublication.hxx \
        UserAgent.hxx \
        ConversationParticipantAssignment.hxx \
        RemoteParticipant.hxx \
        RemoteIMPagerParticipant.hxx \
        RemoteIMSessionParticipant.hxx \
        RemoteIMSessionParticipantDialogSet.hxx \
        MediaResourceParticipant.hxx \
        BridgeMixer.hxx \
        HandleTypes.hxx \
        MediaStreamEvent.hxx \
        ConversationProfile.hxx \
        ConversationManager.hxx \
        DtmfEvent.hxx

if USE_SIPXTAPI
nobase_reconinclude_HEADERS += sdp/SdpMediaLine.hxx \
        sdp/SdpCodec.hxx \
        sdp/SdpHelperResip.hxx \
        sdp/SdpCandidatePair.hxx \
        sdp/SdpCandidate.hxx \
        sdp/Sdp.hxx \
        SipXMediaStackAdapter.hxx \
        SipXMediaInterface.hxx \
        SipXConversation.hxx \
        SipXBridgeMixer.hxx \
        SipXParticipant.hxx \
        SipXLocalParticipant.hxx \
        SipXMediaResourceParticipant.hxx \
        SipXRemoteParticipant.hxx \
        SipXRemoteParticipantDialogSet.hxx \
        FlowManagerSipXSocket.hxx
endif

if USE_KURENTO
nobase_reconinclude_HEADERS += KurentoBridgeMixer.hxx \
        KurentoConversation.hxx \
        KurentoMediaStackAdapter.hxx \
        KurentoParticipant.hxx \
        KurentoRemoteParticipantDialogSet.hxx \
        KurentoRemoteParticipant.hxx
endif

if USE_GSTREAMER
nobase_reconinclude_HEADERS += GstBridgeMixer.hxx \
        GstConversation.hxx \
        GstMediaStackAdapter.hxx \
        GstParticipant.hxx \
        GstRemoteParticipantDialogSet.hxx \
        GstRemoteParticipant.hxx
endif

if USE_LIBWEBRTC
nobase_reconinclude_HEADERS += LibWebRTCBridgeMixer.hxx \
        LibWebRTCConversation.hxx \
        LibWebRTCMediaStackAdapter.hxx \
        LibWebRTCParticipant.hxx \
        LibWebRTCRemoteParticipantDialogSet.hxx \
        LibWebRTCRemoteParticipant.hxx
endif

##############################################################################
# 
# The Vovida Software License, Version 1.0 
# Copyright (c) 2021, Daniel Pocock https://danielpocock.com
# Copyright (c) 2021, Software Freedom Institute https://softwarefreedom.institute
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
