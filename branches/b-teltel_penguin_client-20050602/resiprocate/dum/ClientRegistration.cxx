#include <iterator>

#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/BaseCreator.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/dum/InternalRemoveRegistrationBindingsMessage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientRegistrationHandle
ClientRegistration::getHandle()
{
   return ClientRegistrationHandle(mDum, getBaseHandle().getId());
}

ClientRegistration::ClientRegistration(DialogUsageManager& dum,
                                       DialogSet& dialogSet,
                                       SipMessage& request)
   : NonDialogUsage(dum, dialogSet),
     mLastRequest(request),
     mTimerSeq(0),
     mState(mLastRequest.exists(h_Contacts) ? Adding : Querying),
     mEndWhenDone(false),
     mExpires(0),
     mQueuedState(None)
{
   // If no Contacts header, this is a query
   if (mLastRequest.exists(h_Contacts))
   {
      mMyContacts = mLastRequest.header(h_Contacts);
   }
   mNetworkAssociation.setDum(&dum);
}

ClientRegistration::~ClientRegistration()
{
   DebugLog ( << "ClientRegistration::~ClientRegistration" );
   mDialogSet.mClientRegistration = 0;
}

void
ClientRegistration::addBinding(const NameAddr& contact)
{
   addBinding(contact, mDialogSet.getUserProfile()->getDefaultRegistrationTime());
}

SipMessage&
ClientRegistration::tryModification(ClientRegistration::State state)
{
   if (mState != Registered)
   {
      if (mQueuedState != None)
      {
         WarningLog (<< "Trying to modify bindings when another request is already queued");
         throw UsageUseException("Queuing multiple requests for Registration Bindings", __FILE__,__LINE__);
      }

      mQueuedRequest = mLastRequest;
      mQueuedState = state;

      return mQueuedRequest;
   }

   assert(mQueuedState == None);
   mState = state;

   return mLastRequest;
}

void
ClientRegistration::addBinding(const NameAddr& contact, int registrationTime)
{
   SipMessage& next = tryModification(Adding);
   mMyContacts.push_back(contact);

   next.header(h_Contacts) = mMyContacts;
   next.header(h_Expires).value() = registrationTime;
   next.header(h_CSeq).sequence()++;
   // caller prefs

   if (mQueuedState == None)
   {
      mDum.send(next);
   }
}

void
ClientRegistration::removeBinding(const NameAddr& contact)
{
   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }

   SipMessage& next = tryModification(Removing);
   for (NameAddrs::iterator i=mMyContacts.begin(); i != mMyContacts.end(); i++)
   {
      if (i->uri() == contact.uri())
      {
         mMyContacts.erase(i);

         next.header(h_Contacts) = mMyContacts;
         next.header(h_Expires).value() = 0;
         next.header(h_CSeq).sequence()++;

         if (mQueuedState == None)
         {
            mDum.send(next);
         }

         return;
      }
   }

   // !jf! What state are we left in now?
   throw Exception("No such binding", __FILE__, __LINE__);
}

void
ClientRegistration::removeAll(bool stopRegisteringWhenDone)
{
   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }
   SipMessage& next = tryModification(Removing);

   mAllContacts.clear();
   mMyContacts.clear();

   NameAddr all;
   all.setAllContacts();
   next.header(h_Contacts).clear();
   next.header(h_Contacts).push_back(all);
   next.header(h_Expires).value() = 0;
   next.header(h_CSeq).sequence()++;
   mEndWhenDone = stopRegisteringWhenDone;

   if (mQueuedState == None)
   {
      mDum.send(next);
   }
}

void
ClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   InfoLog (<< "Removing binding (sync)");

   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }
   SipMessage& next = tryModification(Removing);

   for (NameAddrs::iterator i=mMyContacts.begin(); i != mMyContacts.end(); i++)
   {
      i->param(p_expires) = 0;
   }

   next.header(h_Contacts) = mMyContacts;
   next.remove(h_Expires);
   next.header(h_CSeq).sequence()++;

   // !jf! is this ok if queued
   mEndWhenDone = stopRegisteringWhenDone;

   if (mQueuedState == None)
   {
      mDum.send(next);
   }
}

void ClientRegistration::removeMyBindingsAsync(bool stopRegisteringWhenDone)  // !polo! async.
{
   InfoLog (<< "Removing binding (async)");
   mDum.post(new InternalRemoveRegistrationBindingsMessage(getHandle(), stopRegisteringWhenDone));
}

void ClientRegistration::stopRegistering()
{
   //timers aren't a concern, as DUM checks for Handle validity before firing.
   delete this;
}

void
ClientRegistration::requestRefresh(int expires)
{
   InfoLog (<< "requesting refresh of " << *this);
   
   assert (mState == Registered);
   mState = Refreshing;
   mLastRequest.header(h_CSeq).sequence()++;
   if(expires > 0)
   {
      mLastRequest.header(h_Expires).value() = expires;
   }
   mDum.send(mLastRequest);
}

const NameAddrs&
ClientRegistration::myContacts()
{
   return mMyContacts;
}

const NameAddrs&
ClientRegistration::allContacts()
{
   return mAllContacts;
}

int
ClientRegistration::whenExpires() const
{
// !cj! - TODO - I'm supisious these time are getting confused on what units they are in 
   UInt64 now = Timer::getTimeMs() / 1000;
   UInt64 ret = mExpires - now;
   return (int)ret;
}

void
ClientRegistration::end()
{
   removeMyBindings(true);
}

std::ostream& 
ClientRegistration::dump(std::ostream& strm) const
{
   strm << "ClientRegistration " << mLastRequest.header(h_From).uri();
   return strm;
}

void
ClientRegistration::dispatch(const SipMessage& msg)
{
   try
   {
      // !jf! there may be repairable errors that we can handle here
      assert(msg.isResponse());
      int keepAliveTime = mDialogSet.getUserProfile()->getKeepAliveTime();
      if(keepAliveTime > 0)
      {
         mNetworkAssociation.update(msg, keepAliveTime);
      }
      const int& code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         // throw it away
         return;
      }
      else if (code < 300) // success
      {
         //Profile* profile = mDum.getMasterProfile();

         // !jf! consider what to do if no contacts
         // !ah! take list of ctcs and push into mMy or mOther as required.

         if (msg.exists(h_Contacts))
         {
            mAllContacts = msg.header(h_Contacts);

            // make timers to re-register
            int expiry = INT_MAX;
            const Via& via = msg.header(h_Vias).front();
            for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin();
                 it != msg.header(h_Contacts).end(); it++)
            {
               if (it->uri().host() == via.sentHost() &&
                  it->uri().port() == via.sentPort() &&
                  it->exists(p_expires))
               {
                  expiry = resipMin(it->param(p_expires), expiry);
               }
            }
            if (expiry == INT_MAX)
            {
               if (msg.exists(h_Expires))
               {
                  expiry = msg.header(h_Expires).value();
               }
            }
            if (expiry != INT_MAX)
            {
               int exp = Helper::aBitSmallerThan(expiry);
               mExpires = exp + Timer::getTimeMs() / 1000;
               mDum.addTimer(DumTimeout::Registration,
                             exp,
                             getBaseHandle(),
                             ++mTimerSeq);
            }
         }

         switch (mState)
         {
            case Querying:
            case Adding:
               mState = Registered;
               mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
               break;

            case Removing:
               //mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
               mDum.mClientRegistrationHandler->onRemoved(getHandle());
               InfoLog (<< "Finished removing registration " << *this << " mEndWhenDone=" << mEndWhenDone);
               if (mEndWhenDone)
               {
                  // !kh!
                  // stopRegistering() deletes 'this'
                  // furthur processing makes no sense
                  //assert(mQueuedState == None);
                  stopRegistering();
                  return;
               }
               break;

            case Registered:
            case Refreshing:
               mState = Registered;
               break;

            default:
               break;
         }

         if (mQueuedState != None)
         {
            InfoLog (<< "Sending queued request: " << mQueuedRequest);
            mState = mQueuedState;
            mQueuedState = None;
            mLastRequest = mQueuedRequest;
            mDum.send(mLastRequest);
         }
      }
      else
      {
         if (code == 423) // interval too short
         {
            int maxRegistrationTime = mDialogSet.getUserProfile()->getDefaultMaxRegistrationTime();
            if (msg.exists(h_MinExpires) && 
                (maxRegistrationTime == 0 || msg.header(h_MinExpires).value() < maxRegistrationTime)) // If maxRegistrationTime is enabled, then check it
            {
               mLastRequest.header(h_Expires).value() = msg.header(h_MinExpires).value();
               mLastRequest.header(h_CSeq).sequence()++;
               mDum.send(mLastRequest);
               return;
            }
         }
         else if (code == 408)
         {
            int retry = mDum.mClientRegistrationHandler->onRequestRetry(getHandle(), 0, msg);
            
            if (retry < 0)
            {
               DebugLog(<< "Application requested failure on Retry-After");
            }
            else if (retry == 0)
            {
               DebugLog(<< "Application requested immediate retry on 408");
               
               mLastRequest.header(h_CSeq).sequence()++;
               mDum.send(mLastRequest);
               return;
            }
            else
            {
               DebugLog(<< "Application requested delayed retry on 408: " << retry);
               mExpires = 0;
               mDum.addTimer(DumTimeout::RegistrationRetry, 
                             retry, 
                             getBaseHandle(),
                             ++mTimerSeq);       
               return;
            }
         }
         
         mDum.mClientRegistrationHandler->onFailure(getHandle(), msg);

         // Retry if Profile setting is set
         if (mDialogSet.getUserProfile()->getDefaultRegistrationRetryTime() > 0 &&
             (mState == Adding || mState == Refreshing) &&
             !mEndWhenDone)
         {
             unsigned int retryInterval = mDialogSet.getUserProfile()->getDefaultRegistrationRetryTime();
             if (msg.exists(h_RetryAfter))
             {
                 // Use retry interval from error response
                 retryInterval = msg.header(h_RetryAfter).value();
             }
             mExpires = 0;
             mDum.addTimer(DumTimeout::RegistrationRetry,
                           retryInterval,
                           getBaseHandle(),
                           ++mTimerSeq);
             InfoLog( << "Registration error " << code << " for " << msg.header(h_To) << ", retrying in " << retryInterval << " seconds.");
             return;
         }

         // assume that if a failure occurred, the bindings are gone
         if (mEndWhenDone)
         {
            mDum.mClientRegistrationHandler->onRemoved(getHandle());
         }
         delete this;
      }
   }
   catch(BaseException& e)
   {
      InfoLog( << "Exception in ClientRegistration::dispatch: "  <<  e.getMessage());
      mDum.mClientRegistrationHandler->onFailure(getHandle(), msg);
      delete this;
   }
}

void
ClientRegistration::dispatch(const DumTimeout& timer)
{
   switch(timer.type())
   {
      case DumTimeout::Registration:
         // If you happen to be Adding/Updating when the timer goes off, you should just ignore
         // it since a new timer will get added when the 2xx is received.
         if (timer.seq() == mTimerSeq && mState == Registered)
         {
            if (!mMyContacts.empty())
            {
               requestRefresh();
            }
         }
         break;

      case DumTimeout::RegistrationRetry:
         if (timer.seq() == mTimerSeq)
         {
            assert(mState == Adding || mState == Refreshing);

            // Resend last request
            mLastRequest.header(h_CSeq).sequence()++;
            mDum.send(mLastRequest);
         }
         break;
      default:
         break;
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
