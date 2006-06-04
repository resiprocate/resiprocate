#include <algorithm>
#include <iterator>

#include "resip/stack/Helper.hxx"
#include "resip/dumer/ClientRegistration.hxx"
#include "resip/dumer/PrdUseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Random.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientRegistration::ClientRegistration()
   : NonDialogPrd(),
     mTimerSeq(0),
     mState(AddingOrQuerying),
     mEndWhenDone(false),
     mUserRefresh(false),
     mExpires(0),
     mQueuedState(None)
{
   // ?jpr? Handle NetworkAssociation
   mNetworkAssociation.setDum(&dum);
}

ClientRegistration::~ClientRegistration()
{
   DebugLog ( << "ClientRegistration::~ClientRegistration" );
   mDialogSet.mClientRegistration = 0;

   // !dcm! Will not interact well with multiple registrations from the same AOR
   getUserProfile()->setServiceRoute(NameAddrs());
}

SipMessage& 
ClientRegistration::initialize(const NameAddr& target)
{
   initialize(target, mUserProfile->getDefaultRegistrationTime());
}

SipMessage& 
ClientRegistration::initialize(const NameAddr& target, int registrationTime)
{
   // Initialize the mLastRequest member
   makeInitialRequest(target, target, REGISTER);

   mLastRequest.header(h_RequestLine).uri().user() = Data::Empty;
   mLastRequest.header(h_Expires).value() = registrationTime;

   if (mUserProfile->getRinstanceEnabled())
   {
      mLastRequest.header(h_Contacts).front().uri().param(p_rinstance) = Random::getCryptoRandomHex(8);  // !slg! poor mans instance id so that we can tell which contacts are ours - to be replaced by gruu someday
   }

   if (mUserProfile->getMethodsParamEnabled())
   {
      mLastRequest.header(h_Contacts).front().param(p_methods) = mPrd.getMasterProfile()->getAllowedMethodsData();
   }
   
   DebugLog ( << "RegistrationCreator::RegistrationCreator: " << mLastRequest);   
   
   // add instance parameter to the contact for gruu !cj! TODO 

   // store caller prefs in Contact

   return mLastRequest;
}

void
ClientRegistration::addBinding(const NameAddr& contact)
{
   addBinding(contact, mUserProfile->getDefaultRegistrationTime());
}

SipMessage&
ClientRegistration::tryModification(ClientRegistration::State state)
{
   if (mState != Registered)
   {
      if(mState != RetryAdding && mState != RetryRefreshing)
      {
         if (mQueuedState != None)
         {
            WarningLog (<< "Trying to modify bindings when another request is already queued");
            throw PrdUseException("Queuing multiple requests for Registration Bindings", __FILE__,__LINE__);
         }

         mQueuedRequest = mLastRequest;
         mQueuedState = state;

         return mQueuedRequest;
      }
      else
      {
          ++mTimerSeq;  // disable retry timer
      }
   }

   assert(mQueuedState == None);
   mState = state;

   return mLastRequest;
}

void
ClientRegistration::addBinding(const NameAddr& contact, int registrationTime)
{
   SipMessage& next = tryModification(AddingOrQuerying);
   mMyContacts.push_back(contact);

   if(mUserProfile->getRinstanceEnabled())
   {
      mMyContacts.back().uri().param(p_rinstance) = Random::getCryptoRandomHex(8);  // !slg! poor mans instance id so that we can tell which contacts are ours - to be replaced by gruu someday
   }

   next.header(h_Contacts) = mMyContacts;
   next.header(h_Expires).value() = registrationTime;
   next.header(h_CSeq).sequence()++;
   // caller prefs

   if (mQueuedState == None)
   {
      send(next);
   }
}

void
ClientRegistration::removeBinding(const NameAddr& contact)
{
   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw PrdUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
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
            send(next);
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
      throw PrdUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
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
      send(next);
   }
}

void
ClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   InfoLog (<< "Removing binding");

   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw PrdUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }

   if (mMyContacts.empty())
   {
      WarningLog (<< "No bindings to remove");
      throw PrdUseException("No bindings to remove", __FILE__,__LINE__);
   }

   SipMessage& next = tryModification(Removing);

   NameAddrs myContacts = mMyContacts;
   mMyContacts.clear();

   for (NameAddrs::iterator i=myContacts.begin(); i != myContacts.end(); i++)
   {
      i->param(p_expires) = 0;
   }

   next.header(h_Contacts) = myContacts;
   next.remove(h_Expires);
   next.header(h_CSeq).sequence()++;

   // !jf! is this ok if queued
   mEndWhenDone = stopRegisteringWhenDone;

   if (mQueuedState == None)
   {
      send(next);
   }
}

void ClientRegistration::stopRegistering()
{
   //timers aren't a concern, as DUM checks for Handle validity before firing.
   unmanage();
}

void
ClientRegistration::requestRefresh(int expires)
{
    // Set flag so that handlers get called for success/failure
    mUserRefresh = true;
    internalRequestRefresh(expires);
}

void
ClientRegistration::internalRequestRefresh(int expires)
{
   InfoLog (<< "requesting refresh of " << *this);
   
   assert (mState == Registered);
   mState = Refreshing;
   mLastRequest->header(h_CSeq).sequence()++;
   if(expires > 0)
   {
      mLastRequest->header(h_Expires).value() = expires;
   }

   send(mLastRequest);
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
   strm << "ClientRegistration " << mLastRequest->header(h_From).uri();
   return strm;
}

void
ClientRegistration::protectedDispatch(const SipMessage& msg)
{
   try
   {
      // !jf! there may be repairable errors that we can handle here
      assert(msg.isResponse());

      /*   !slg! Needs to be fixed
      if(msg.isExternal())
      {
         const Data& receivedTransport = msg.header(h_Vias).front().transport();
         int keepAliveTime = 0;
         if(receivedTransport == Symbols::TCP ||
            receivedTransport == Symbols::TLS ||
            receivedTransport == Symbols::SCTP)
         {
            keepAliveTime = mDialogSet.getUserProfile()->getKeepAliveTimeForStream();
         }
         else
         {
            keepAliveTime = mDialogSet.getUserProfile()->getKeepAliveTimeForDatagram();
         }

         if(keepAliveTime > 0)
         {
            mNetworkAssociation.update(msg, keepAliveTime);
         }
      }*/

      const int& code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         // throw it away
         return;
      }
      else if (code < 300) // success
      {
         try
         {
            if (msg.exists(h_ServiceRoutes))
            {
               InfoLog(<< "Updating service route: " << Inserter(msg.header(h_ServiceRoutes)));
               mUserProfile->setServiceRoute(msg.header(h_ServiceRoutes));
            }
            else
            {
               InfoLog(<< "Clearing service route (" << Inserter(getUserProfile()->getServiceRoute()) << ")");
               mUserProfile->setServiceRoute(NameAddrs());
            }
         }
         catch(BaseException &e)
         {
            InfoLog(<< "Error Parsing Service Route:" << e);
         }
         

         // !jf! consider what to do if no contacts
         // !ah! take list of ctcs and push into mMy or mOther as required.

         // If no Contacts header, this is a query
         if (mInitialRequest->exists(h_Contacts))
         {
            mMyContacts = mInitialRequest->header(h_Contacts);
         }

         if (msg.exists(h_Contacts))
         {
            mAllContacts = msg.header(h_Contacts);

            // make timers to re-register
            int expiry = INT_MAX;
            //!dcm! -- should do set intersection with my bindings and walk that
            //small size, n^2, don't care
            if (mUserProfile->getRinstanceEnabled())
            {
               int fallbackExpiry = INT_MAX;  // Used if no contacts found with our rinstance - this can happen if proxies do not echo back the rinstance property correctly
               for (NameAddrs::iterator itMy = mMyContacts.begin(); itMy != mMyContacts.end(); itMy++)
               {
                  for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin(); it != msg.header(h_Contacts).end(); it++)
                  {
                     try
                     {
                        if(it->exists(p_expires))
                        {
                           // rinstace parameter is added to contacts created by this client, so we can 
                           // use it to determine which contacts in the 200 response are ours.  This
                           // should eventually be replaced by gruu stuff.
                           if (it->uri().exists(p_rinstance) && 
                               it->uri().param(p_rinstance) == itMy->uri().param(p_rinstance))
                           {
                              expiry = resipMin(it->param(p_expires), expiry);
                           }
                           else
                           {
                              fallbackExpiry = resipMin(it->param(p_expires), fallbackExpiry);
                           }
                        }
                     }
                     catch(ParseBuffer::Exception& e)
                     {
                        DebugLog(<< "Ignoring unparsable contact in REG/200: " << e);
                     }
                  }
                  if(expiry == INT_MAX)  // if we didn't find a contact with our rinstance, then use the fallbackExpiry
                  {
                     expiry = fallbackExpiry;
                  }
               }
            }
            else
            {
               for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin();
                    it != msg.header(h_Contacts).end(); it++)
               {
                  //add to boolean exp. but needs testing
                  //std::find(myContacts().begin(), myContacts().end(), *it) != myContacts().end()
                  if (it->exists(p_expires))
                  {
                     try
                     {
                        expiry = resipMin(it->param(p_expires), expiry);
                     }
                     catch(ParseBuffer::Exception& e)
                     {
                        DebugLog(<< "Ignoring unparsable contact in REG/200: " << e);
                     }
                  }
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
               mPrd.addTimer(DumTimeout::Registration,
                             exp,
                             getBaseHandle(),
                             ++mTimerSeq);
            }
         }

         switch (mState)
         {
            case AddingOrQuerying:
               mState = Registered;
               onSuccess(msg);
               break;

            case Removing:
               //mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
               onRemoved(msg);
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
               if(mUserRefresh)
               {
                   mUserRefresh = false;
                   onSuccess(msg);
               }
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
            send(mLastRequest);
         }
      }
      else
      {
         if((mState == AddingOrQuerying || mState == Refreshing) && !mEndWhenDone)
         {
            if (code == 423) // interval too short
            {
               int maxRegistrationTime = mUserProfile->getDefaultMaxRegistrationTime();
               if (msg.exists(h_MinExpires) && 
                   (maxRegistrationTime == 0 || msg.header(h_MinExpires).value() < maxRegistrationTime)) // If maxRegistrationTime is enabled, then check it
               {
                  mLastRequest->header(h_Expires).value() = msg.header(h_MinExpires).value();
                  mLastRequest->header(h_CSeq).sequence()++;
                  send(mLastRequest);
                  return;
               }
            }
            else if (code == 408)
            {
               int retry = onRequestRetry(0, msg);
            
               if (retry < 0)
               {
                  DebugLog(<< "Application requested failure on Retry-After");
               }
               else if (retry == 0)
               {
                  DebugLog(<< "Application requested immediate retry on 408");
               
                  mLastRequest->header(h_CSeq).sequence()++;
                  send(mLastRequest);
                  return;
               }
               else
               {
                  DebugLog(<< "Application requested delayed retry on 408: " << retry);
                  mExpires = 0;
                  switch(mState)
                  {
                  case AddingOrQuerying:
                     mState = RetryAdding;
                     break;
                  case Refreshing:
                     mState = RetryRefreshing;
                     break;
                  default:
                     assert(false);
                     break;
                  }
                  mPrd.addTimer(DumTimeout::RegistrationRetry, 
                                retry, 
                                getBaseHandle(),
                                ++mTimerSeq);       
                  return;
               }
            }
         }
         
         onFailure(msg);

         // Retry if Profile setting is set
         if (mUserProfile->getDefaultRegistrationRetryTime() > 0 &&
             (mState == AddingOrQuerying || mState == Refreshing) &&
             !mEndWhenDone)
         {
            unsigned int retryInterval = mUserProfile)->getDefaultRegistrationRetryTime();
            if (msg.exists(h_RetryAfter))
            {
               // Use retry interval from error response
               retryInterval = msg.header(h_RetryAfter).value();
            }
            mExpires = 0;
            switch(mState)
            {
            case AddingOrQuerying:
               mState = RetryAdding;
               break;
            case Refreshing:
               mState = RetryRefreshing;
               break;
            default:
                assert(false);
               break;
            }

            mPrd.addTimer(DumTimeout::RegistrationRetry,
                          retryInterval,
                          getBaseHandle(),
                          ++mTimerSeq);
            InfoLog( << "Registration error " << code << " for " << msg.header(h_To) << ", retrying in " << retryInterval << " seconds.");
            return;
         }

         // assume that if a failure occurred, the bindings are gone
         if (mEndWhenDone)
         {
            onRemoved(msg);
         }
         unmanage();
      }
   }
   catch(BaseException& e)
   {
      InfoLog( << "Exception in ClientRegistration::dispatch: "  <<  e.getMessage());
      onFailure(msg);
      unmanage();
   }
}

void
ClientRegistration::protectedDispatch(const DumTimeout& timer)
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
               internalRequestRefresh();
            }
         }
         break;

      case DumTimeout::RegistrationRetry:
         if (timer.seq() == mTimerSeq)
         {
            switch(mState)
            {
            case RetryAdding:
               mState = AddingOrQuerying;
               break;
            case RetryRefreshing:
               mState = Refreshing;
               break;
            default:
              assert(false);
              break;
            }

            // Resend last request
            mLastRequest->header(h_CSeq).sequence()++;
            send(mLastRequest);
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
