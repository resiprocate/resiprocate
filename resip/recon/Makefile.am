# $Id$

EXTRA_DIST = readme.txt
EXTRA_DIST += *.vcproj
EXTRA_DIST += *.sln
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += doc

SUBDIRS = .
SUBDIRS += test
SUBDIRS += MOHParkServer

#AM_CXXFLAGS = -DUSE_ARES

lib_LTLIBRARIES = librecon.la
librecon_la_LIBADD = ../../reflow/libreflow.la
librecon_la_LIBADD += ../../reTurn/client/libreTurnClient.la
librecon_la_LIBADD += ../dum/libdum.la
librecon_la_LIBADD += ../stack/libresip.la
librecon_la_LIBADD += ../../rutil/librutil.la
librecon_la_LIBADD += -lsipXmediaProcessing
librecon_la_LIBADD += -lsipXmedia
librecon_la_LIBADD += -lsipXsdp
librecon_la_LIBADD += -lsipXport
librecon_la_LIBADD += @LIBSSL_LIBADD@ -lsrtp
librecon_la_LIBADD += @LIBPTHREAD_LIBADD@
librecon_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

librecon_la_SOURCES = BridgeMixer.cxx \
        Conversation.cxx \
        ConversationManager.cxx \
        ConversationParticipantAssignment.cxx \
        ConversationProfile.cxx \
        DefaultDialogSet.cxx \
        DtmfEvent.cxx \
        FlowManagerSipXSocket.cxx \
        LocalParticipant.cxx \
        MediaEvent.cxx \
                MediaInterface.cxx \
        MediaResourceParticipant.cxx \
        MediaResourceCache.cxx \
        MediaStreamEvent.cxx \
        Participant.cxx \
        ReconSubsystem.cxx \
        RelatedConversationSet.cxx \
        RemoteParticipant.cxx \
        RemoteParticipantDialogSet.cxx \
        SipXHelper.cxx \
        UserAgent.cxx \
        UserAgentClientSubscription.cxx \
        UserAgentDialogSetFactory.cxx \
        UserAgentMasterProfile.cxx \
        UserAgentRegistration.cxx \
        UserAgentServerAuthManager.cxx \
        sdp/Sdp.cxx \
        sdp/SdpCandidate.cxx \
        sdp/SdpCandidatePair.cxx \
        sdp/SdpCodec.cxx \
        sdp/SdpHelperResip.cxx \
        sdp/SdpMediaLine.cxx

reconincludedir = $(includedir)/recon
nobase_reconinclude_HEADERS = ReconSubsystem.hxx \
        LocalParticipant.hxx \
        MediaResourceCache.hxx \
        RelatedConversationSet.hxx \
        UserAgentServerAuthManager.hxx \
        ConversationManagerCmds.hxx \
        UserAgentMasterProfile.hxx \
        RemoteParticipantDialogSet.hxx \
        SipXHelper.hxx \
        Conversation.hxx \
        UserAgentDialogSetFactory.hxx \
        sdp/SdpMediaLine.hxx \
        sdp/SdpCodec.hxx \
        sdp/SdpHelperResip.hxx \
        sdp/SdpCandidatePair.hxx \
        sdp/SdpCandidate.hxx \
        sdp/Sdp.hxx \
        MediaInterface.hxx \
        UserAgentCmds.hxx \
        UserAgentRegistration.hxx \
        MediaEvent.hxx \
        DefaultDialogSet.hxx \
        Participant.hxx \
        UserAgentClientSubscription.hxx \
        UserAgent.hxx \
        ConversationParticipantAssignment.hxx \
        RemoteParticipant.hxx \
        MediaResourceParticipant.hxx \
        BridgeMixer.hxx \
        HandleTypes.hxx \
        MediaStreamEvent.hxx \
        FlowManagerSipXSocket.hxx \
        ConversationProfile.hxx \
        ConversationManager.hxx \
        DtmfEvent.hxx

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
