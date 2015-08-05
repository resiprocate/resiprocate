#include "repro/stateAgents/PresenceSubscriptionHandler.hxx"
#include "repro/stateAgents/PresenceServer.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/Dispatcher.hxx"

#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/PublicationPersistenceManager.hxx>
#include <resip/dum/RegistrationPersistenceManager.hxx>
#include <resip/dum/ServerPublication.hxx>
#include <resip/stack/GenericPidfContents.hxx>
#include <rutil/ResipAssert.h>
#include <rutil/Logger.hxx>

using namespace repro;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

PresenceSubscriptionHandler::PresenceSubscriptionHandler(resip::DialogUsageManager& dum,
                                                         repro::Dispatcher* userDispatcher,
                                                         bool presenceUsesRegistrationState,
                                                         bool presenceNotifyClosedStateForNonPublishedUsers)
  : InMemorySyncRegDbHandler(InMemorySyncRegDbHandler::AllChanges),
    InMemorySyncPubDbHandler(InMemorySyncPubDbHandler::AllChanges),
    mDum(dum), 
    mPublicationDb(dynamic_cast<InMemorySyncPubDb*>(dum.getPublicationPersistenceManager())),
    mRegistrationDb(dynamic_cast<InMemorySyncRegDb*>(dum.getRegistrationPersistenceManager())),
    mPresenceUsesRegistrationState(presenceUsesRegistrationState),
    mPresenceNotifyClosedStateForNonPublishedUsers(presenceNotifyClosedStateForNonPublishedUsers),
    mUserDispatcher(userDispatcher)
{
   resip_assert(mPublicationDb);
   resip_assert(mRegistrationDb);
   if (mPresenceUsesRegistrationState)
   {
      mRegistrationDb->addHandler(this);
   }
   mPublicationDb->addHandler(this);
}

PresenceSubscriptionHandler::~PresenceSubscriptionHandler()
{
   if (mPresenceUsesRegistrationState)
   {
      mRegistrationDb->removeHandler(this);
   }
   mPublicationDb->removeHandler(this);
}

void 
PresenceSubscriptionHandler::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
{
    InfoLog(<< "PresenceSubscriptionHandler::onNewSubscription: msg=" << std::endl << sub);
    notifyPresence(h, true /* sendAcceptReject? */);
}

void PresenceSubscriptionHandler::onRefresh(ServerSubscriptionHandle h, const SipMessage& sub)
{
   h->send(h->accept(200));

   // Check to see if registration state changed
   if (mPresenceUsesRegistrationState)
   {
      Uri aor("sip:" + h->getDocumentKey());
      UInt64 maxExpires = 0;
      bool isRegistered = mRegistrationDb->aorIsRegistered(aor, &maxExpires);
      InfoLog(<< "PresenceSubscriptionHandler::onRefresh: aor=" << aor << ", registered=" << isRegistered << ", maxRegExpires=" << maxExpires);
      if (!checkRegistrationStateChanged(aor, isRegistered, maxExpires))
      {
         SharedPtr<SipMessage> notify = h->neutralNotify();
         if (maxExpires && isRegistered)
         {
            adjustNotifyExpiresTime(*notify.get(), maxExpires);
         }
         h->send(notify);
      }
   }
   else
   {
      h->send(h->neutralNotify());
   }
}

void 
PresenceSubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                         ServerPublicationHandle publication, 
                                         const Contents* contents,
                                         const SecurityAttributes* attrs)
{
   if (contents)
   {
      InfoLog(<< "PresenceSubscriptionHandler::onPublished: docKey=" << associated->getDocumentKey() << ", contents=" << std::endl << *contents);
   }
   else
   {
      InfoLog(<< "PresenceSubscriptionHandler::onPublished: no contents, docKey=" << associated->getDocumentKey());
   }
   // We don't need to call notifyPresence here, since we are subscribed to changes in the InMemorySyncPubDb.  
   // This allows up to notify for PUBLISHes that arrive via a sync operation as well.
   //notifyPresence(associated, false /* sendAcceptReject? */);
}


void 
PresenceSubscriptionHandler::onTerminated(ServerSubscriptionHandle h)
{
   InfoLog(<< "PresenceSubscriptionHandler::onTerminated: docKey=" << h->getDocumentKey());
}

void 
PresenceSubscriptionHandler::onError(ServerSubscriptionHandle h, const SipMessage& msg)
{
    InfoLog(<< "PresenceSubscriptionHandler::onError: docKey=" << h->getDocumentKey() << ", msg=" << std::endl << msg);
}

void 
PresenceSubscriptionHandler::notifyPresence(resip::ServerSubscriptionHandle h, bool sendAcceptReject)
{
   try
   {
      Uri aor("sip:" + h->getDocumentKey());
      if (!mPresenceUsesRegistrationState)
      {
         DebugLog(<< "PresenceSubscriptionHandler::notifyPresence: attempting to notify published presence for aor=" << aor);
         if (!sendPublishedPresence(h, sendAcceptReject))
         {            
            notifyPresenceNoPublication(h, sendAcceptReject, aor, mRegistrationDb->aorIsRegistered(aor), 0 /* maxRegExpires - not needed in this case */);
         }
      }
      else
      {
         UInt64 maxExpires = 0;
         bool isRegistered = mRegistrationDb->aorIsRegistered(aor, &maxExpires);
         if (isRegistered) 
         {
            mOnlineAors.insert(aor);
            DebugLog(<< "PresenceSubscriptionHandler::notifyPresence: attempting to notify published presence for aor=" << aor);
            if (!sendPublishedPresence(h, sendAcceptReject))
            {
               // Fabricate a simple presence update based on registration state
               fabricateSimplePresence(h, sendAcceptReject, aor, true /* online? */, maxExpires);
            }
         }
         else
         {
            notifyPresenceNoPublication(h, sendAcceptReject, aor, isRegistered, maxExpires);
         }
      }
   }
   catch (BaseException& ex)
   {
      ErrLog(<< "PresenceSubscriptionHandler::notifyPresence: problem creating aor for registration lookup: " << ex);
      if (sendAcceptReject)
      {
         h->send(h->reject(500));
      }
   }
}

void 
PresenceSubscriptionHandler::notifyPresenceNoPublication(resip::ServerSubscriptionHandle h, bool sendAcceptReject, const Uri& aor, bool isRegistered, UInt64 regMaxExpires)
{
   DebugLog(<< "PresenceSubscriptionHandler::notifyPresenceNoPublication: no publication for aor=" << aor << ", registered=" << isRegistered);
   // First we can look in the InMemory Registration database for the user - if they are there, we know they exist
   // and we don't need to do an async lookup
   if (isRegistered)
   {
      if (mPresenceUsesRegistrationState)
      {
         mOnlineAors.insert(aor);
         fabricateSimplePresence(h, sendAcceptReject, aor, true /* online? */, regMaxExpires);
      }
      else
      {
         continueNotifyPresenceAfterUserExistsCheck(h, sendAcceptReject, aor, true /* userExists? */);
      }
   }
   else
   {
      mOnlineAors.erase(aor);  // remove now, since we might have an immediate unregister coming in before this aysnc finishes and we want to avoid sending 2 closed states
         
      // Do async lookup of user from User tables
      PresenceUserExists* async = new PresenceUserExists(mDum, this, h, sendAcceptReject, aor);
      std::auto_ptr<ApplicationMessage> app(async);
      mUserDispatcher->post(app);
   }
}

void 
PresenceUserExists::executeCommand() 
{ 
   if (mServerSubscriptionHandle.isValid())
   {
      mSubscriptionHandler->continueNotifyPresenceAfterUserExistsCheck(mServerSubscriptionHandle, mSendAcceptReject, mAor, mUserExists);
   }
}

void
PresenceSubscriptionHandler::continueNotifyPresenceAfterUserExistsCheck(resip::ServerSubscriptionHandle h, bool sendAcceptReject, const Uri& aor, bool userExists)
{
   DebugLog(<< "PresenceSubscriptionHandler::continueNotifyPresenceAfterUserExistsCheck: aor=" << aor << ", userExists=" << userExists);
   if (!mPresenceUsesRegistrationState)
   {
      if (sendAcceptReject)
      {
         if (userExists)
         {
            if (mPresenceNotifyClosedStateForNonPublishedUsers)
            {
                fabricateSimplePresence(h, sendAcceptReject, aor, false /* online? */, 0 /* regMaxExpires not need in this case */);
            }
            else
            {
                h->send(h->reject(480));
            }
         }
         else
         {
            h->send(h->reject(404));
         }
      }
      else
      {
          if (mPresenceNotifyClosedStateForNonPublishedUsers)
          {
              fabricateSimplePresence(h, sendAcceptReject, aor, false /* online? */, 0 /* regMaxExpires not need in this case */);
          }
          else
          {
              h->end(NoResource);
          }
      }
   }
   else
   {
      if (userExists)
      {
         mOnlineAors.erase(aor);

         // we only get here if user wasn't registered, so hardcode online to false
         fabricateSimplePresence(h, sendAcceptReject, aor, false /* online? */, 0 /* regMaxExpires not need in this case */);
      }
      else if (sendAcceptReject)
      {
         h->send(h->reject(404));
      }
   }
}

bool 
PresenceSubscriptionHandler::sendPublishedPresence(resip::ServerSubscriptionHandle h, bool sendAcceptReject)
{
   GenericPidfContents pidf;
   bool result = mPublicationDb->getMergedETags(h->getEventType(), h->getDocumentKey(), *this, &pidf);
   if (result)
   {
      if (sendAcceptReject)
      {
         h->setSubscriptionState(Active);
         h->send(h->accept(200));
      }
      h->send(h->update(&pidf));
   }
   return result;
}

const UInt32 ReSubGraceTime = 32;  // Somewhat arbitrary - using SIP transaction timeout
void
PresenceSubscriptionHandler::adjustNotifyExpiresTime(SipMessage& notify, UInt64 regMaxExpires)
{
   resip_assert(notify.exists(h_SubscriptionState));
   resip_assert(notify.header(h_SubscriptionState).exists(p_expires));

   // The following is an effort to catch timed out / expired registrations
   // We are reducing the subscription expiration time to try and get the client to refresh the
   // subscription soon after the registration may have expired.  Since rfc 3261 recommends
   // refreshing using Helper::aBitSmallerThan logic - we assume the clients will do the same
   // and we calculate a time using the inverse calculation (including a small grace period).
   // If the registration does expire without a refresh, then the rebsub should arrive shortly
   // after and when it does we will check the reg state.  
   // TODO - a better solution would be to implement timers for registration expirations, then
   //        just notify immediately on expiration.  However this is not very straight forward
   //        as we must also consider sync'd registrations and need a place to start all these
   //        timers.

   UInt32 timeUntillRegExpires = (UInt32)(regMaxExpires - Timer::getTimeSecs());
   // Scale up expiration time used in Notify to be the opposite of aBitSmallerThan + some grace time
   UInt32 scaledUpTimeSoSubRefreshComesAfter = resipMax(timeUntillRegExpires + 5 + ReSubGraceTime, ((timeUntillRegExpires * 10) / 9) + ReSubGraceTime); 

   // RFC3265 - says we can shorten the expiration but not increase it - so using resipMin
   notify.header(h_SubscriptionState).param(p_expires) = resipMin(scaledUpTimeSoSubRefreshComesAfter, notify.header(h_SubscriptionState).param(p_expires));
}

void 
PresenceSubscriptionHandler::fabricateSimplePresence(ServerSubscriptionHandle h, bool sendAcceptReject, const resip::Uri& aor, bool online, UInt64 regMaxExpires)
{
   InfoLog(<< "PresenceSubscriptionHandler::fabricateSimplePresence: aor=" << aor << ", online=" << online << ", maxRegExpires=" << regMaxExpires);

   // Fabricate a simple presence update based on registration state
   GenericPidfContents pidf;
   pidf.setEntity(aor);
   pidf.setSimplePresenceTupleNode(h->getDocumentKey(), online, GenericPidfContents::generateNowTimestampData());
   if (sendAcceptReject)
   {
      h->setSubscriptionState(Active);
      h->send(h->accept(200));
   }
   resip::SharedPtr<SipMessage> notify = h->update(&pidf);
   if (regMaxExpires && online)
   {
      adjustNotifyExpiresTime(*notify.get(), regMaxExpires);
   }
   h->send(notify);
}

bool 
PresenceSubscriptionHandler::mergeETag(Contents* eTagDest, Contents* eTagSrc, bool isFirst)
{
   GenericPidfContents* destPidf = dynamic_cast<GenericPidfContents*>(eTagDest);
   GenericPidfContents* srcPidf = dynamic_cast<GenericPidfContents*>(eTagSrc);
   if (destPidf && srcPidf)
   {
      if (isFirst)
      {
         // If this is the first doc, there is nothing to merge - just use operator=
         *destPidf = *srcPidf;
      }
      else
      {
         destPidf->merge(*srcPidf);
      }
      return true;
   }
   return false;
}

// Used to get registrationChanged to run in DumThread context
class repro::PresenceServerRegStateChangeCommand : public DumCommandAdapter
{
public:
   PresenceServerRegStateChangeCommand(PresenceSubscriptionHandler& handler, const resip::Uri& aor, bool registered, UInt64 regMaxExpires)
      : mHandler(handler), mAor(aor), mRegistered(registered), mRegMaxExpires(regMaxExpires) {}

   virtual void executeCommand()
   {
      mHandler.checkRegistrationStateChanged(mAor, mRegistered, mRegMaxExpires);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "PresenceServerRegStateChangeCommand: aor=" << mAor << ", registered=" << mRegistered;
   }
private:
   PresenceSubscriptionHandler& mHandler;
   resip::Uri mAor;
   bool mRegistered;
   UInt64 mRegMaxExpires;
};

// Used to fabricate a presence update to all applicable subscriptions
class repro::PresenceServerSubscriptionRegFunctor
{
public:
   PresenceServerSubscriptionRegFunctor(PresenceSubscriptionHandler& handler, const resip::Uri& aor, bool online, UInt64 regMaxExpires) :
      mHandler(handler), mAor(aor), mOnline(online), mRegMaxExpires(regMaxExpires) {}
   virtual ~PresenceServerSubscriptionRegFunctor() { }

   virtual void operator()(ServerSubscriptionHandle h)
   {
      if (mOnline)
      {
         if (!mHandler.sendPublishedPresence(h, false /* sendAcceptReject */))
         {
            // Fabricate a simple presence update based on registration state
            mHandler.fabricateSimplePresence(h, false /* sendAcceptReject */, mAor, true /* online? */, mRegMaxExpires);
         }
      }
      else
      {
         mHandler.fabricateSimplePresence(h, false /* sendAcceptReject */, mAor, false /* online? */, mRegMaxExpires);
      }
   }
private:
   PresenceSubscriptionHandler& mHandler;
   Uri mAor;
   bool mOnline;
   UInt64 mRegMaxExpires;
};

bool 
PresenceSubscriptionHandler::checkRegistrationStateChanged(const resip::Uri& aor, bool registered, UInt64 regMaxExpires)
{
   // Last reported online?
   bool online = mOnlineAors.find(aor) != mOnlineAors.end();
   bool stateChanged = false;
   // Check if state has changed or not
   if (online && !registered)
   {
      stateChanged = true;
      mOnlineAors.erase(aor);
      online = false;
      DebugLog(<< "PresenceSubscriptionHandler::checkRegistrationStateChanged: registration changed for aor=" << aor << ", no longer registered");
   }
   else if (!online && registered)
   {
      stateChanged = true;
      mOnlineAors.insert(aor);
      online = true;
      DebugLog(<< "PresenceSubscriptionHandler::checkRegistrationStateChanged: registration changed for aor=" << aor << ", now registered");
   }
   if (stateChanged)
   {
      PresenceServerSubscriptionRegFunctor functor(*this, aor, online, regMaxExpires);
      Data aorData = aor.user() + "@" + aor.host();
      mDum.applyToServerSubscriptions<PresenceServerSubscriptionRegFunctor>(aorData, Symbols::Presence, functor);
   }
   else
   {
       DebugLog(<< "PresenceSubscriptionHandler::checkRegistrationStateChanged: registration state unchanged for aor=" << aor);
   }
   return stateChanged;
}

void 
PresenceSubscriptionHandler::onAorModified(const resip::Uri& aor, const ContactList& contacts)
{
   bool registered = false;
   UInt64 maxExpirationTime = 0;
   UInt64 now = Timer::getTimeSecs();

   // iterate contacts and see if registered or not
   ContactList::const_iterator it = contacts.begin();
   for (; it != contacts.end(); it++)
   {
      if (it->mRegExpires > now)
      {
         registered = true;
         maxExpirationTime = resipMax(maxExpirationTime, it->mRegExpires);
      }
   }

   DebugLog(<< "PresenceSubscriptionHandler::onAorModified: registration updated aor=" << aor << ", registered=" << registered);

   // Need to move the remaining processing to the DUM thread
   mDum.post(new PresenceServerRegStateChangeCommand(*this, aor, registered, maxExpirationTime));
}

// Used to notify presence update to all applicable subscriptions
class repro::PresenceServerSubscriptionFunctor
{
public:
   PresenceServerSubscriptionFunctor(PresenceSubscriptionHandler& handler) : mHandler(handler) {}
   virtual ~PresenceServerSubscriptionFunctor() { }

   virtual void operator()(ServerSubscriptionHandle h)
   {
      mHandler.notifyPresence(h, false /* sendAcceptReject? */);
   }
private:
   PresenceSubscriptionHandler& mHandler;
};

// Used to send notifies in DumThread context
class repro::PresenceServerDocStateChangeCommand : public DumCommandAdapter
{
public:
   PresenceServerDocStateChangeCommand(PresenceSubscriptionHandler& handler, const resip::Data& documentKey)
      : mHandler(handler), mDocumentKey(documentKey) {}

   virtual void executeCommand()
   {
      mHandler.notifySubscriptions(mDocumentKey);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "PresenceServerDocStateChangeCommand: aor=" << mDocumentKey;
   }
private:
   PresenceSubscriptionHandler& mHandler;
   resip::Data mDocumentKey;
};

// Used to generate a timer expirey in DumThread context
class repro::PresenceServerCheckDocExpiredCommand : public DumCommandAdapter
{
public:
   PresenceServerCheckDocExpiredCommand(PresenceSubscriptionHandler& handler, const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated)
      : mHandler(handler), mDocumentKey(documentKey), mETag(eTag), mLastUpdated(lastUpdated) {}

   virtual void executeCommand()
   {
      mHandler.checkExpired(mDocumentKey, mETag, mLastUpdated);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "PresenceServerCheckDocExpiredCommand: aor=" << mDocumentKey;
   }
private:
   PresenceSubscriptionHandler& mHandler;
   resip::Data mDocumentKey;
   resip::Data mETag;
   UInt64 mLastUpdated;
};

void
PresenceSubscriptionHandler::notifySubscriptions(const Data& documentKey)
{
   PresenceServerSubscriptionFunctor functor(*this);
   mDum.applyToServerSubscriptions<PresenceServerSubscriptionFunctor>(documentKey, Symbols::Presence, functor);
}

void PresenceSubscriptionHandler::checkExpired(const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated)
{
    //DebugLog(<< "PresenceSubscriptionHandler::checkExpired: docKey=" << documentKey << ", tag=" << eTag << ", lastUpdated=" << lastUpdated);
    mPublicationDb->checkExpired(Symbols::Presence, documentKey, eTag, lastUpdated);
}

void 
PresenceSubscriptionHandler::onDocumentModified(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes)
{
   if (eventType == Symbols::Presence)
   {
      if (contents != 0)  // If contents is 0, we have a pub refresh and we don't need to send notifies out)
      {
         DebugLog(<< "PresenceSubscriptionHandler::onDocumentModified: aor=" << documentKey << ", eTag=" << eTag);
         // Signal DUM thread to send notifies for this change
         mDum.post(new PresenceServerDocStateChangeCommand(*this, documentKey));
      }
      // If this is a sync'd publication then set a timer to see when it times out - this is needed if sync is broken when expirey happens
      // when timer expires then see if document has expired and if so then generate mHandler.notifySubscriptions(mDocumentKey);
      if (sync)
      {
         UInt64 expiresSeconds = expirationTime - Timer::getTimeSecs();
         if (expiresSeconds > 0)
         {
            //DebugLog(<< "PresenceSubscriptionHandler::onDocumentModified: starting check expired timer for sync'd publication, docKey=" << documentKey << ", tag=" << eTag << ", lastUpdated=" << lastUpdated << ", timerExpirey=" << expiresSeconds);
            mDum.getSipStack().post(std::auto_ptr<resip::ApplicationMessage>(new PresenceServerCheckDocExpiredCommand(*this, documentKey, eTag, lastUpdated)), (unsigned int)expiresSeconds, &mDum);
         }
      }
   }
}

void
PresenceSubscriptionHandler::onDocumentRemoved(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated)
{
   if (eventType == Symbols::Presence)
   {
      DebugLog(<< "PresenceSubscriptionHandler::onDocumentRemoved: aor=" << documentKey << ", eTag=" << eTag);
      // Signal DUM thread to send notifies for this change
      mDum.post(new PresenceServerDocStateChangeCommand(*this, documentKey));
   }
}

/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
*/
/*
* vi: set shiftwidth=3 expandtab:
*/
