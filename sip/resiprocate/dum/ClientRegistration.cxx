#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/BaseCreator.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"

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
     mEndWhenDone(false)
{
   // If no Contacts header, this is a query
   if (mLastRequest.exists(h_Contacts))
   {
      mMyContacts = mLastRequest.header(h_Contacts);
   }
}

ClientRegistration::~ClientRegistration()
{
   DebugLog ( << "ClientRegistration::~ClientRegistration" );
   mDialogSet.mClientRegistration = 0;
}

void 
ClientRegistration::addBinding(const NameAddr& contact)
{
   addBinding(contact, mDum.getProfile()->getDefaultRegistrationTime());
}

void 
ClientRegistration::addBinding(const NameAddr& contact, int registrationTime)
{
   mMyContacts.push_back(contact);
   mLastRequest.header(h_Contacts) = mMyContacts;
   mLastRequest.header(h_Expires).value() = registrationTime;
   mLastRequest.header(h_CSeq).sequence()++;
   // caller prefs

   if (mState != Registered)
   {
      throw UsageUseException("Can't add binding when already modifying registration bindings", __FILE__,__LINE__);
   }

   mState = Adding;
   mDum.send(mLastRequest);
}

void 
ClientRegistration::removeBinding(const NameAddr& contact)
{
   if (mState != Registered)
   {
      throw UsageUseException("Can't remove binding when already modifying registration bindings", __FILE__,__LINE__);
   }

   for (NameAddrs::iterator i=mMyContacts.begin(); i != mMyContacts.end(); i++)
   {
      if (i->uri() == contact.uri())
      {
         mMyContacts.erase(i);

         mLastRequest.header(h_Contacts) = mMyContacts;
         mLastRequest.header(h_Expires).value() = 0;
         mLastRequest.header(h_CSeq).sequence()++;
         mState = Removing;
         mDum.send(mLastRequest);
         
         return;
      }
   }

   throw Exception("No such binding", __FILE__, __LINE__);
}

void 
ClientRegistration::removeAll(bool stopRegisteringWhenDone)
{
   if (mState != Registered)
   {
      throw UsageUseException("Can't remove bindings when already modifying registration bindings", __FILE__,__LINE__);
   }

   mAllContacts.clear();
   mMyContacts.clear();
   
   NameAddr all;
   all.setAllContacts();
   mLastRequest.header(h_Contacts).clear();
   mLastRequest.header(h_Contacts).push_back(all);
   mLastRequest.header(h_Expires).value() = 0;
   mLastRequest.header(h_CSeq).sequence()++;
   mEndWhenDone = stopRegisteringWhenDone;
   
   mState = Removing;
   mDum.send(mLastRequest);
}

void 
ClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   InfoLog (<< "Removing binding");

   if (mState != Registered)
   {
      InfoLog (<< "Trying to remove bindings when modifying: " << mState);
      throw UsageUseException("Can't remove binding when already modifying registration bindings", __FILE__,__LINE__);
   }

   for (NameAddrs::iterator i=mMyContacts.begin(); i != mMyContacts.end(); i++)
   {
      i->param(p_expires) = 0;
   }

   mLastRequest.header(h_Contacts) = mMyContacts;
   mLastRequest.remove(h_Expires);
   mLastRequest.header(h_CSeq).sequence()++;
   mEndWhenDone = stopRegisteringWhenDone;
   mState = Removing;
   mDum.send(mLastRequest);
}

void
ClientRegistration::end()
{
   stopRegistering();
}

void 
ClientRegistration::stopRegistering()
{
   //timers aren't a concern, as DUM checks for Handle validity before firing.
   delete this;     
}

void 
ClientRegistration::requestRefresh()
{
   mLastRequest.header(h_CSeq).sequence()++;
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

void 
ClientRegistration::dispatch(const SipMessage& msg)
{
   try
   {
      // !jf! there may be repairable errors that we can handle here
      assert(msg.isResponse());
      const int& code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         // throw it away
         return;
      }
      else if (code < 300) // success
      {
         //Profile* profile = mDum.getProfile();
         
         // !jf! consider what to do if no contacts
         // !ah! take list of ctcs and push into mMy or mOther as required.
         
         if (msg.exists(h_Contacts))
         {
            mAllContacts = msg.header(h_Contacts);

            // make timers to re-register
            int expiry = INT_MAX;            
            for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin(); 
                 it != msg.header(h_Contacts).end(); it++)
            {
               if(it->exists(p_expires))
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
               mDum.addTimer(DumTimeout::Registration, 
                             Helper::aBitSmallerThan(expiry),
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
               if (mEndWhenDone)
               {
                  stopRegistering();
               }
               break;

            case Registered:
               break;
               
         }
      }
      else
      {
         if (code == 423) // interval too short
         {
            // maximum 1 day 
            // !ah! why max check? -- profile?
            if (msg.exists(h_MinExpires) && msg.header(h_MinExpires).value()  < 86400) 
            {
               mLastRequest.header(h_Expires).value() = msg.header(h_MinExpires).value();
               mLastRequest.header(h_CSeq).sequence()++;
               mDum.send(mLastRequest);
               return;
            }
         }
         mDum.mClientRegistrationHandler->onFailure(getHandle(), msg);

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
   // If you happen to be Adding/Updating when the timer goes off, you should just ignore
   // it since a new timer will get added when the 2xx is received. 
   if (timer.seq() == mTimerSeq && mState == Registered)
   {
      if (!mMyContacts.empty())
      {
         requestRefresh();
      }
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
