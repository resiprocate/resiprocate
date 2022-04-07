#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "UserAgentDialogSetFactory.hxx"
#include "RemoteParticipant.hxx"
#include "RemoteIMSessionParticipantDialogSet.hxx"
#include "DefaultDialogSet.hxx"
#include "MediaStackAdapter.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/dum/AppDialogSet.hxx>
#include <rutil/WinLeakCheck.hxx>

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace recon;
using namespace resip;
using namespace std;


UserAgentDialogSetFactory::UserAgentDialogSetFactory(ConversationManager& conversationManager) :
    mConversationManager(conversationManager)
{
}

AppDialogSet* 
UserAgentDialogSetFactory::createAppDialogSet(DialogUsageManager& dum,
                                              const SipMessage& msg)
{
   switch(msg.method())
   {
   case INVITE:
   {
      SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
      if (sdp)
      {
         for (auto it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
         {
            if (it->name() == "message")
            {
               // If there is a message media line, then we assume no audio/video are present and we create an RemoteIMSessionParticipantDialogSet
               return mConversationManager.createRemoteIMSessionParticipantDialogSetInstance();
            }
         }
      }
      return mConversationManager.getMediaStackAdapter().createRemoteParticipantDialogSetInstance();
      break;
   }
   default:
      return new DefaultDialogSet(mConversationManager);
      break;
   }
}


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc.  http://www.sipspectrum.com
 Copyright (c) 2007-2008, Plantronics, Inc.
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
