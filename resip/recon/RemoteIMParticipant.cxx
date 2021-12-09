#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "RemoteIMParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/Contents.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ServerPagerMessage.hxx>
#include <resip/dum/ClientPagerMessage.hxx>

#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

// For incoming
RemoteIMParticipant::RemoteIMParticipant(ParticipantHandle partHandle,
                                         ConversationManager& conversationManager) : 
   IMParticipantBase(false /* prependSenderInfoToIMs? */),
   Participant(partHandle, conversationManager),
   AppDialogSet(conversationManager.getUserAgent()->getDialogUsageManager()),
   mDum(conversationManager.getUserAgent()->getDialogUsageManager()),
   mNumOutstandingSends(0),
   mDelayedDestroyPending(false)
{
   InfoLog(<< "RemoteIMParticipant created, handle=" << mHandle);
   mConversationProfile = nullptr;
}

// For createRemoteIMParticipant
RemoteIMParticipant::RemoteIMParticipant(ParticipantHandle partHandle, ConversationManager& conversationManager,
                                         const NameAddr& destination, std::shared_ptr<ConversationProfile> conversationProfile) :
   IMParticipantBase(true /* prependSenderInfoToIMs? */),
   Participant(partHandle, conversationManager),
   AppDialogSet(conversationManager.getUserAgent()->getDialogUsageManager()),
   mDum(conversationManager.getUserAgent()->getDialogUsageManager())
{
   InfoLog(<< "RemoteIMParticipant created, handle=" << mHandle);
   mRemoteUri = destination;
   mRemoteAorNoPort = mRemoteUri.uri().getAorNoPort();

   if (conversationProfile == nullptr)
   {
      mConversationProfile = mConversationManager.getUserAgent()->getDefaultOutgoingConversationProfile();
   }
   else
   {
      mConversationProfile = conversationProfile;
   }

   mLocalAorNoPort = mConversationProfile->getDefaultFrom().uri().getAorNoPort();
}

RemoteIMParticipant::~RemoteIMParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "RemoteIMParticipant destroyed, handle=" << mHandle);
}

void
RemoteIMParticipant::destroyParticipant()
{
   if (mNumOutstandingSends > 0)
   {
      // If we have some outstanding sends, delay the destruction...
      mDelayedDestroyPending = true;
      return;
   }

   try
   {
      if (mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->send(mServerPagerMessageHandle->reject(410 /* Gone */));  // This will also delete the ServerPagerMessage object
      }
      if (mClientPagerMessageHandle.isValid())
      {
         mClientPagerMessageHandle->end();
      }
      else
      {
         delete this;
      }
   }
   catch (BaseException& e)
   {
      WarningLog(<< "RemoteIMParticipant::destroyParticipant exception: " << e);
   }
   catch (...)
   {
      WarningLog(<< "RemoteIMParticipant::destroyParticipant unknown exception");
   }
}

void
RemoteIMParticipant::sendInstantMessage(std::unique_ptr<Contents> contents)
{
   if (!mClientPagerMessageHandle.isValid())
   {
      // If we don't have a ClientPagerMessageHandle yet then we need to create it
      mClientPagerMessageHandle = mDum.makePagerMessage(mRemoteUri, mConversationProfile, this);
   }
   mClientPagerMessageHandle->page(std::move(contents));
   mNumOutstandingSends++;  // Increment until onSuccess or onFailure responses arrive
}

void
RemoteIMParticipant::accept()
{
   try
   {
      if (mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->send(mServerPagerMessageHandle->accept());

         // notify of new message on participant
         bool relay = false;
         if (mHandle) relay = mConversationManager.onReceiveIMFromParticipant(mHandle, mInitialIncomingMessage);

         // Relay to others in our conversations, if requested to do so
         if (relay)
         {
            relayInstantMessageToConversations(mInitialIncomingMessage);
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteIMParticipant::accept exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteIMParticipant::accept unknown exception");
   }
}

void 
RemoteIMParticipant::reject(unsigned int rejectCode)
{
   try
   {
      if (mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->send(mServerPagerMessageHandle->reject(rejectCode));
         assert(!mClientPagerMessageHandle.isValid());
         delete this;
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteIMParticipant::reject exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteIMParticipant::reject unknown exception");
   }
}

bool 
RemoteIMParticipant::doesMessageMatch(resip::SipMessage message)
{
   Data messageToAor = message.header(h_To).uri().getAorNoPort();
   Data messageFromAor = message.header(h_From).uri().getAorNoPort();
   bool matches = mLocalAorNoPort == messageToAor && mRemoteAorNoPort == messageFromAor;

   DebugLog(<< "RemoteIMParticipant::doesMessageMatch: partHandle=" << mHandle << ", LocalAor=" << mLocalAorNoPort << ", msgTo=" << messageToAor << ", RemoteAor=" << mRemoteAorNoPort << ", msgFrom=" << messageFromAor << (matches ? " - YES" : " - NO"));
   
   return matches;
}

////////////////////////////////////////////////////////////////////////////////
// ClientPagerMessageHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
RemoteIMParticipant::onSuccess(ClientPagerMessageHandle h, const SipMessage& status)
{
   // Note: there is an odd case where we have delayed destruction (ie: mDelayedDestroyPending)
   //       and we execute the delayed destroy before mNumOutstandingSends reaches 0 (because of a 
   //       onFailure).  In this case we may have artificially set mNumOutstandingSends to 0 in the 
   //       onFailure callback, and it's possible that another onSuccess or onFailure callback arrives
   //       before DUM can complete the destruction of this dialogset.
   if (mNumOutstandingSends > 0)
   {
      mNumOutstandingSends--;
   }
   if (mDelayedDestroyPending && mNumOutstandingSends == 0)
   {
      // Reset mDelayedDestroyPending flag to avoid possibily calling destoryParticipant a 2nd time
      mDelayedDestroyPending = false;

      // All sends are complete.  If we delayed a destroy because messages were pending, it's now safe to destroy.
      destroyParticipant();
   }
   // TODO: Do we want/need a callback for this?
}

void
RemoteIMParticipant::onFailure(ClientPagerMessageHandle h, const SipMessage& status, std::unique_ptr<Contents> contents)
{
   // Note: there is an odd case where we have delayed destruction (ie: mDelayedDestroyPending)
   //       and we execute the delayed destroy before mNumOutstandingSends reaches 0 (because of a 
   //       onFailure).  In this case we may have artificially set mNumOutstandingSends to 0 in the 
   //       onFailure callback, and it's possible that another onSuccess or onFailure callback arrives
   //       before DUM can complete the destruction of this dialogset.
   if (mNumOutstandingSends > 0)
   {
      mNumOutstandingSends--;
   }

   if (mHandle) mConversationManager.onParticipantSendIMFailure(mHandle, status, std::move(contents));

   // Regardless of whether all sends are complete, we execute any delayed destroy now, since one
   // error likely means more, and we don't want to delay destruction for a long time (ie: while a bunch of 
   // sends timeout and fail).
   if (mDelayedDestroyPending)
   {
      // Reset mNumOutstandingSends so that destoryParticipant will go through
      mNumOutstandingSends = 0;

      // Reset mDelayedDestroyPending flag to avoid a subsequent onFailure callback, arriving before 
      // destruction is complete, from calling destoryParticipant a 2nd time
      mDelayedDestroyPending = false; 

      destroyParticipant();
   }
}


////////////////////////////////////////////////////////////////////////////////
// ServerPagerMessageHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
RemoteIMParticipant::onMessageArrived(ServerPagerMessageHandle h, const SipMessage& msg)
{
   InfoLog(<< "RemoteIMParticipant::onMessageArrived: handle=" << mHandle << ", " << msg.brief());

   mServerPagerMessageHandle = h;

   // Set Remote Display Name if not already set.  Note: we only do this when receiving an inbound message, since
   // that's the only time we can expect that From header display name will be properly filled out.
   if (mRemoteDisplayName.empty())
   {
      mRemoteDisplayName = msg.header(h_From).displayName();
      if (mRemoteDisplayName.empty())
      {
         // If no display name present then use user field
         mRemoteDisplayName = msg.header(h_From).uri().user();
      }
   }

   if (mConversationProfile == nullptr)
   {
      // This is the first message
      mConversationProfile = std::dynamic_pointer_cast<ConversationProfile>(h->getUserProfile());
      assert(mConversationProfile != nullptr);
      mInitialIncomingMessage = msg;
      mRemoteUri = NameAddr(msg.header(h_From).uri());
      mRemoteAorNoPort = mRemoteUri.uri().getAorNoPort();
      mLocalAorNoPort = msg.header(h_To).uri().getAorNoPort();

      // notify of new participant
      if (mHandle) mConversationManager.onIncomingIMParticipant(mHandle, msg, *mConversationProfile);
   }
   else
   {
      // Automatically accept any subsequent messages received after initial one
      h->send(h->accept());

      // notify of new message on existing participant
      bool relay = false;
      if (mHandle) relay = mConversationManager.onReceiveIMFromParticipant(mHandle, msg);
      
      // Relay to others in our conversations, if requested to do so
      if (relay)
      {
         relayInstantMessageToConversations(msg);
      }
   }
}

void
RemoteIMParticipant::relayInstantMessageToConversations(const SipMessage& msg)
{
   ConversationMap::iterator it;
   for (it = mConversations.begin(); it != mConversations.end(); it++)
   {
      it->second->relayInstantMessageToRemoteParticipants(mHandle, mRemoteDisplayName, msg);
   }
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
