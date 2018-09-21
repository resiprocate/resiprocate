
#include "B2BCallManager.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <iostream>
#include <fstream>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <AppSubsystem.hxx>

#include "MyUserAgent.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace recon;
using namespace reconserver;

B2BCall::B2BCall(const recon::ConversationHandle& conv, const recon::ParticipantHandle& a, const recon::ParticipantHandle b, const resip::SipMessage& msg, const resip::Data& originZone, const resip::Data& destinationZone, const resip::Data& b2bCallID)
    : mConversation(conv),
      mPartA(a),
      mPartB(b),
      mOriginZone(originZone),
      mDestinationZone(destinationZone),
      mB2BCallID(b2bCallID),
      mCaller(msg.header(h_From).uri().getAor()),
      mCallee(msg.header(h_RequestLine).uri().getAor()),
      mResponseCode(-1),
      mStart(ResipClock::getTimeMs()),
      mConnect(0),
      mFinish(0)
{
}

B2BCallManager::B2BCallManager(MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config, resip::SharedPtr<B2BCallLogger> b2bCallLogger)
   : MyConversationManager(false, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false),
     mB2BCallLogger(b2bCallLogger)
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

   Data usersFilename;
   config.getConfigValue("B2BUAUsersFilename", usersFilename);
   if(!usersFilename.empty());
   {
      loadUserCredentials(usersFilename);
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
      if(partHandle == call->participantA())
      {
         target = call->participantB();
      }
      else if(partHandle == call->participantB())
      {
         target = call->participantA();
      }
      Data tone(Data("tone:") + buttons[dtmf] + Data(";duration=") + Data(duration) + Data(";participant-only=") + Data(target));
      Uri _tone(tone);
      StackLog(<< "sending tone to conversation: " << _tone);
      ConversationManager::createMediaResourceParticipant(call->conversation(), _tone);
   }
   else
   {
      WarningLog(<< "Participant " << partHandle << " sent a DTMF signal, not known in any existing call");
   }
}

void
B2BCallManager::onIncomingParticipant(ParticipantHandle partHandleA, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onIncomingParticipant: handle=" << partHandleA << "auto=" << autoAnswer << " msg=" << msg.brief());
   mRemoteParticipantHandles.push_back(partHandleA);
   // Create a new conversation for each new participant
   ConversationHandle conv = createConversation();
   addParticipant(conv, partHandleA);
   const Uri& reqUri = msg.header(h_RequestLine).uri();
   SharedPtr<ConversationProfile> profile;
   bool internalSource = isSourceInternal(msg);
   Data originZoneName;
   Data destinationZoneName;
   if(internalSource)
   {
      originZoneName = "internal";
      destinationZoneName = "external";
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
      originZoneName = "external";
      destinationZoneName = "internal";
      SharedPtr<ConversationProfile> internalProfile = getInternalConversationProfile();
      profile.reset(new ConversationProfile(*internalProfile.get()));
      // Look up the user in mUsers
      const Data& callerUri = msg.header(h_From).uri().getAor();
      // If found in mUsers, put the credentials into the profile
      if(mUsers.find(callerUri) != mUsers.end())
      {
         const Data& callerUsername = msg.header(h_From).uri().user();
         const Data& callerRealm = msg.header(h_From).uri().host();
         DebugLog(<<"found credential for authenticating " << callerUri << " in realm " << callerRealm << " and added it to user profile");
         profile->clearDigestCredentials();
         profile->setDigestCredential(callerRealm, callerUsername, mUsers.find(callerUri)->second.mPassword);
      }
      else
      {
         DebugLog(<<"didn't find individual credential for authenticating " << callerUri);
      }
   }
   static ExtensionHeader h_X_CID("X-CID");
   std::multimap<Data,Data> extraHeaders;
   Data b2bCallID;
   if(msg.exists(h_X_CID) && internalSource)
   {
      const ParserContainer<resip::StringCategory>& pc = msg.header(h_X_CID);
      ParserContainer<StringCategory>::const_iterator v = pc.begin();
      for( ; v != pc.end(); v++)
      {
         b2bCallID = v->value();
         extraHeaders.insert(std::pair<Data,Data>(h_X_CID.getName(), b2bCallID));
      }
   }
   else
   {
      // no correlation header exists in incoming message, copy A-leg Call-ID header to B-leg h_X_CID
      b2bCallID = msg.header(h_CallId).value();
      extraHeaders.insert(std::pair<Data,Data>(h_X_CID.getName(), b2bCallID));
   }
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
   ParticipantHandle partHandleB = ConversationManager::createRemoteParticipant(conv, NameAddr(reqUri), ForkSelectAutomatic, _profile, extraHeaders);
   SharedPtr<B2BCall> call(new B2BCall(conv, partHandleA, partHandleB, msg, originZoneName, destinationZoneName, b2bCallID));
   mCallsByConversation[call->conversation()] = call;
   mCallsByParticipant[call->participantA()] = call;
   mCallsByParticipant[call->participantB()] = call;
}

void
B2BCallManager::onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantTerminated: handle=" << partHandle);
   if(mCallsByParticipant.find(partHandle) != mCallsByParticipant.end())
   {
      SharedPtr<B2BCall> call = mCallsByParticipant[partHandle];
      if(partHandle == call->participantB())
      {
         rejectParticipant(call->participantA(), statusCode);
      }
      else
      {
         rejectParticipant(call->participantB(), statusCode);
      }
      destroyConversation(call->conversation());
      mCallsByParticipant.erase(call->participantA());
      mCallsByParticipant.erase(call->participantB());
      mCallsByConversation.erase(call->conversation());
      call->onFinish(statusCode);
      if(mB2BCallLogger.get())
      {
         mB2BCallLogger->log(call);
      }
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
      if(call->participantB() == partHandle)
      {
         alertParticipant(call->participantA(), false);
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
      if(call->participantB() == partHandle)
      {
         answerParticipant(call->participantA());
         call->onConnect();
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

void
B2BCallManager::loadUserCredentials(Data filename)
{
   if(!mUsers.empty())
   {
      WarningLog(<<"loadUserCredentials called but mUsers already populated");
      return;
   }

   InfoLog(<< "trying to load user credentials from file " << filename);

   std::ifstream usersFile(filename.c_str());
   if(!usersFile)
   {
      ErrLog(<< "failed to open users file: " << filename << ", aborting");
      throw std::runtime_error("Error opening/reading users file");
   }

   std::string sline;
   while(getline(usersFile, sline))
   {
      Data line(sline);
      Data uri;
      Data password;
      UserCredentials creds;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      const char * anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it

      // Look for end of name
      pb.skipToOneOf("\t");
      pb.data(uri, anchor);
      if(mUsers.find(uri) != mUsers.end())
      {
         ErrLog(<< "URI '" << uri << "' repeated in users file");
         throw std::runtime_error("URI repeated in users file");
      }
      pb.skipChar('\t');

      if(!pb.eof())
      {
         pb.skipWhitespace();
         if(pb.eof())
            break;

         anchor = pb.position();
         pb.skipToOneOf("\r\n ");
         pb.data(password, anchor);
         if(!password.empty())
         {
            creds.mUsername = uri;
            creds.mPassword = password;
            mUsers[uri] = creds;
            DebugLog(<< "Loaded user URI '" << uri << "'");
         }
         if(!pb.eof())
            pb.skipChar();
      }
   }
   InfoLog(<<"Loaded " << mUsers.size() << " users");
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

