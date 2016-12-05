
#include "B2BCallManager.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <recon/ReconSubsystem.hxx>

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace resip;
using namespace recon;

B2BCallManager::B2BCallManager(MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config)
   : MyConversationManager(false, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false)
{ 
   mB2BUANextHop = config.getConfigData("B2BUANextHop", "", true);
   if(mB2BUANextHop.size() == 0)
   {
      CritLog(<<"Please specify B2BUANextHop");
      throw ConfigParse::Exception("Please specify B2BUANextHop", __FILE__, __LINE__);
   }

   std::set<Data> replicatedHeaderNames;
   if(config.getConfigValue("B2BUAReplicateHeaders", replicatedHeaderNames))
   {
      std::set<Data>::const_iterator it = replicatedHeaderNames.begin();
      for( ; it != replicatedHeaderNames.end(); it++)
      {
         const resip::Data& headerName(*it);
         resip::Headers::Type hType = resip::Headers::getType(headerName.data(), (int)headerName.size());
         if(hType == resip::Headers::UNKNOWN)
         {
            mReplicatedHeaders.push_back(headerName.c_str());
            InfoLog(<<"Will replicate header '"<<headerName<<"'");
         }
         else
         {
            WarningLog(<<"Will not replicate header '"<<headerName<<"', only extension headers permitted");
         }
      }
   }
}

void
B2BCallManager::onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up)
{
   // mappings from integer event codes to character symbols
   static const char* buttons = "0123456789*#ABCD";

   InfoLog(<< "onDtmfEvent: handle=" << partHandle << " tone=" << dtmf << " dur=" << duration << " up=" << up);

   if(!up)
   {
      return;
   }

   if(mCallsByParticipant.find(partHandle) != mCallsByParticipant.end())
   {
      SharedPtr<B2BCall> call = mCallsByParticipant[partHandle];
      if(dtmf > 15)
      {
         WarningLog(<< "Unhandled DTMF code: " << dtmf);
         return;
      }
      int target = 0;
      if(partHandle == call->a)
      {
         target = call->b;
      }
      else if(partHandle == call->b)
      {
         target = call->a;
      }
      Data tone(Data("tone:") + buttons[dtmf] + Data(";duration=") + Data(duration) + Data(";participant-only=") + Data(target));
      Uri _tone(tone);
      StackLog(<< "sending tone to conversation: " << _tone);
      ConversationManager::createMediaResourceParticipant(call->conv, _tone);
   }
   else
   {
      WarningLog(<< "Participant " << partHandle << " sent a DTMF signal, not known in any existing call");
   }
}

void
B2BCallManager::onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onIncomingParticipant: handle=" << partHandle << "auto=" << autoAnswer << " msg=" << msg.brief());
   mRemoteParticipantHandles.push_back(partHandle);
   // Create a new conversation for each new participant
   SharedPtr<B2BCall> call(new B2BCall);
   call->a = partHandle;
   call->conv = createConversation();
   addParticipant(call->conv, call->a);
   const Uri& reqUri = msg.header(h_RequestLine).uri();
   NameAddr newDest("sip:" + reqUri.user() + '@' + mB2BUANextHop);
   std::multimap<Data,Data> extraHeaders;
   std::vector<resip::Data>::const_iterator it = mReplicatedHeaders.begin();
   for( ; it != mReplicatedHeaders.end(); it++)
   {
      const ExtensionHeader h(*it);
      if(msg.exists(h))
      {
         // Replicate the header and value into the outgoing INVITE
         const ParserContainer<StringCategory>& pc = msg.header(h);
         ParserContainer<StringCategory>::const_iterator v = pc.begin();
         for( ; v != pc.end(); v++)
         {
            extraHeaders.insert(std::pair<Data,Data>(h.getName(), v->value()));
         }
      }
   }
   SharedPtr<UserProfile> profile(new ConversationProfile(conversationProfile));
   NameAddr outgoingCaller = conversationProfile.getDefaultFrom();
   outgoingCaller.uri().user() = msg.header(h_From).uri().user();
   outgoingCaller.displayName() = msg.header(h_From).displayName();
   profile->setDefaultFrom(outgoingCaller);
   call->b = ConversationManager::createRemoteParticipant(call->conv, newDest, ForkSelectAutomatic, profile, extraHeaders);
   mCallsByConversation[call->conv] = call;
   mCallsByParticipant[call->a] = call;
   mCallsByParticipant[call->b] = call;
}

void
B2BCallManager::onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantTerminated: handle=" << partHandle);
   if(mCallsByParticipant.find(partHandle) != mCallsByParticipant.end())
   {
      SharedPtr<B2BCall> call = mCallsByParticipant[partHandle];
      if(partHandle == call->b)
      {
         rejectParticipant(call->a, statusCode);
      }
      else
      {
         rejectParticipant(call->b, statusCode);
      }
      destroyConversation(call->conv);
      mCallsByParticipant.erase(call->a);
      mCallsByParticipant.erase(call->b);
      mCallsByConversation.erase(call->conv);
   }
   else
   {
      WarningLog(<< "Participant " << partHandle << " terminated, not known in any existing call");
   }
}
 
void
B2BCallManager::onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantProceeding: handle=" << partHandle << " msg=" << msg.brief());
}

void
B2BCallManager::onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantAlerting: handle=" << partHandle << " msg=" << msg.brief());
   if(mCallsByParticipant.find(partHandle) != mCallsByParticipant.end())
   {
      SharedPtr<B2BCall> call = mCallsByParticipant[partHandle];
      if(call->b == partHandle)
      {
         alertParticipant(call->a, false);
      }
      else
      {
         WarningLog(<<"Unexpected alerting signal from A-leg of call, partHandle = " << partHandle);
      }
   }
   else
   {
      WarningLog(<< "Participant " << partHandle << " alerting, not known in any existing call");
   }
}
    
void
B2BCallManager::onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnected: handle=" << partHandle << " msg=" << msg.brief());
   if(mCallsByParticipant.find(partHandle) != mCallsByParticipant.end())
   {
      SharedPtr<B2BCall> call = mCallsByParticipant[partHandle];
      if(call->b == partHandle)
      {
         answerParticipant(call->a);
      }
      else
      {
         WarningLog(<<"Unexpected connected signal from A-leg of call, partHandle = " << partHandle);
      }
   }
   else
   {
      WarningLog(<< "Participant " << partHandle << " connected, not known in any existing call");
   }
}


/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

