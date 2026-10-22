#if !defined(RemoteIMSessionParticipantDialogSet_hxx)
#define RemoteIMSessionParticipantDialogSet_hxx

#include <map>
#include <list>
#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include "ConversationManager.hxx"
#include "ConversationProfile.hxx"
#include "Participant.hxx"
#include "RemoteParticipantDialogSet.hxx"

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace recon
{
class ConversationManager;
class RemoteParticipant;
class RemoteIMSessionParticipant;

/**
  This class is used by Invite DialogSets used for IM Sessions only (m=message SDP).  Other Invite DialogSets
  are managed by SipXRemoteParticipantDialogSet, and Non-Invite DialogSets are managed by DefaultDialogSet.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteIMSessionParticipantDialogSet : public RemoteParticipantDialogSet
{
public:
   RemoteIMSessionParticipantDialogSet(ConversationManager& conversationManager,
                              ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
                              std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

   virtual ~RemoteIMSessionParticipantDialogSet();

protected:
   virtual bool isAsyncMediaSetup() { return false; }

   virtual void fixUpSdp(resip::SdpContents* sdp) { }

private:
   ConversationManager& mConversationManager;
};

}

#endif


/* ====================================================================

 Copyright (c) 2022, SIP Spectrum, Inc.  http://www.sipspectrum.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
