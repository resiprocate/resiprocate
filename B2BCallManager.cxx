
#include "B2BCallManager.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <recon/ReconSubsystem.hxx>

#include "MyUserAgent.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace resip;
using namespace recon;

B2BCallManager::B2BCallManager(MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config)
   : MyConversationManager(false, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false)
{ 
   config.getConfigValue("B2BUAInternalHosts", mInternalHosts);
   config.getConfigValue("B2BUAInternalTLSNames", mInternalTLSNames);
   mInternalAllPrivate = config.getConfigBool("B2BUAInternalAllPrivate", false);

   if(mInternalHosts.empty() && mInternalTLSNames.empty() && !mInternalAllPrivate)
   {
      WarningLog(<<"None of the options B2BUAInternalHosts, B2BUAInternalTLSNames or B2BUAInternalAllPrivate specified");
   }

   config.getConfigValue("B2BUAInternalMediaAddress", mInternalMediaAddress);
   if(mInternalMediaAddress.empty())
   {
      WarningLog(<<"B2BUAInternalMediaAddress not specified, using same media address for internal and external zones");
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
   SharedPtr<ConversationProfile> profile;
   if(isSourceInternal(msg))
   {
      DebugLog(<<"INVITE request from zone: internal");
      Uri uri(msg.header(h_RequestLine).uri());
      uri.param(p_lr);
      NameAddrs route;
      route = msg.header(h_Routes);
      route.pop_front();  // remove ourselves
      MyUserAgent *ua = dynamic_cast<MyUserAgent*>(getUserAgent());
      resip_assert(ua);
      SharedPtr<ConversationProfile> externalProfile = ua->getDefaultOutgoingConversationProfile();
      profile.reset(new ConversationProfile(*externalProfile.get()));
      profile->setServiceRoute(route);
   }
   else
   {
      DebugLog(<<"INVITE request from zone: external");
      SharedPtr<ConversationProfile> internalProfile = getInternalConversationProfile();
      profile.reset(new ConversationProfile(*internalProfile.get()));
   }
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
   NameAddr outgoingCaller = msg.header(h_From);
   profile->setDefaultFrom(outgoingCaller);
   SharedPtr<UserProfile> _profile(profile);
   call->b = ConversationManager::createRemoteParticipant(call->conv, NameAddr(reqUri), ForkSelectAutomatic, _profile, extraHeaders);
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

resip::SharedPtr<ConversationProfile>
B2BCallManager::getIncomingConversationProfile(const resip::SipMessage& msg, resip::SharedPtr<ConversationProfile> defaultProfile)
{
   DebugLog(<<"getIncomingConversationProfile: defaultProfile.get() == " << defaultProfile.get());
   if(isSourceInternal(msg))
   {
      DebugLog(<<"getIncomingConversationProfile: returning profile for internal call leg");
      return getInternalConversationProfile();
   }
   return defaultProfile;
}

resip::SharedPtr<ConversationProfile>
B2BCallManager::getInternalConversationProfile()
{
   MyUserAgent *ua = dynamic_cast<MyUserAgent*>(getUserAgent());
   resip_assert(ua);
   if(!mInternalMediaAddress.empty())
   {
      SharedPtr<ConversationProfile> p = ua->getConversationProfileByMediaAddress(mInternalMediaAddress);
      if(p.get())
      {
         return p;
      }
   }
   return ua->getDefaultOutgoingConversationProfile();
}

bool
B2BCallManager::isSourceInternal(const SipMessage& msg)
{
   if(mInternalAllPrivate && msg.getSource().isPrivateAddress())
   {
      DebugLog(<<"Matched internal host by IP in private network (RFC 1918 / RFC 4193)");
      return true;
   }

   Data sourceAddr = Tuple::inet_ntop(msg.getSource());
   if(std::find(mInternalHosts.begin(), mInternalHosts.end(), sourceAddr) != mInternalHosts.end())
   {
      DebugLog(<<"Matched internal host by IP: " << sourceAddr);
      return true;
   }

   const std::list<Data>& peerNames = msg.getTlsPeerNames();
   std::list<Data>::const_iterator it = peerNames.begin();
   for( ; it != peerNames.end() ; it++)
   {
      const Data& peerName = *it;
      if(std::find(mInternalTLSNames.begin(), mInternalTLSNames.end(), peerName) != mInternalTLSNames.end())
      {
         DebugLog(<<"Matched internal host by TLS name: " << peerName);
      }
   }

   DebugLog(<<"Didn't match internal host for source " << sourceAddr);
   return false;
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

