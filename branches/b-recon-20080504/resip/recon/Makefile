BUILD := ../build
include $(BUILD)/Makefile.pre

PACKAGES += ASIO RUTIL ARES OPENSSL PTHREAD
TARGET_LIBRARY = librecon
CXXFLAGS += -DBOOST_ALL_NO_LIB -I/usr/local/include/boost-1_34_1 -I../dtls_wrapper -I../reTurn \
            -I../../contrib/dtls-srtp/srtp/include -I../../contrib/dtls-srtp/srtp/crypto/include
          
#Note: This Makefile is not complete
# hard-coded to where the sipX path
#RTPDIR = $(LIBDIR)
#RTPINC = $(RTPDIR)include
#RTPLIB = $(RTPDIR)lib/libsipXmediaProcessing.a -L$(RTPDIR)lib -lsipezRtlLib $(RTPDIR)lib/libsipXmedia.a $(RTPDIR)lib/libsipXtack.a $(RTPDIR)lib/libsipXsdp.a $(RTPDIR)lib/libsipXport.a $(RTPDIR)lib/libpcre.so -lresolv
#RTPLIB = $(RTPDIR)lib/libsipXmediaProcessing.a -L$(RTPDIR)lib $(RTPDIR)lib/libsipXmedia.a $(RTPDIR)lib/libsipXtack.a $(RTPDIR)lib/libsipXsdp.a $(RTPDIR)lib/libsipXport.a $(RTPDIR)lib/libpcre.so -lresolv -ldl
#RTPLIBDIR = $(RTPDIR)lib

LDFLAGS += -ldl

SRC += \
        BridgeMixer.cxx \
        Conversation.cxx \
        ConversationManager.cxx \
        ConversationParticipantAssignment.cxx \
        ConversationProfile.cxx \
        DefaultDialogSet.cxx \
        DtmfEvent.cxx \
        FlowManagerSipXSocket.cxx \
        LocalParticipant.cxx \
        MediaResourceParticipant.cxx \
        MediaResourceCache.cxx \
        MediaStreamEvent.cxx \
        Participant.cxx \
        RelatedConversationSet.cxx \
        RemoteParticipant.cxx \
        RemoteParticipantDialogSet.cxx \
        SdpHelperResip.cxx \
        UserAgent.cxx \
        UserAgentClientSubscription.cxx \
        UserAgentDialogSetFactory.cxx \
        UserAgentMasterProfile.cxx \
        UserAgentRegistration.cxx \
        UserAgentServerAuthManager.cxx \
        UserAgentSubsystem.cxx

include $(BUILD)/Makefile.post


####################################################################
#
# Original contribution Copyright (C) 2008 Plantronics, Inc.
# Provided under the terms of the Vovida Software License, Version 2.0.
#
# The Vovida Software License, Version 2.0 
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
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
# IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
####################################################################
