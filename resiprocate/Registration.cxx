#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <cassert>
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Registration.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

Registration::Registration(const Uri& aor)
   : mAor(aor),
     mFrom(aor), 
     mTimeTillExpiration(3600),
     mState(Initialized)
{
}

Registration::Registration(const Uri& aor, const Uri& contact)
   : mAor(aor),
     mContact(contact),
     mFrom(aor),
     mTimeTillExpiration(3600),
     mState(Initialized)
{
}

Registration::Registration(const Uri& from, const Uri& aor, const Uri& contact)
   : mAor(aor),
     mContact(contact),
     mFrom(from),
     mTimeTillExpiration(3600),
     mState(Initialized)
{
}

void
Registration::setExpiration(int secs)
{
   mTimeTillExpiration = secs;
   if (mState == Active)
   {
      assert(0);
      // !jf! need to reregister now
   }
}

SipMessage&
Registration::getRegistration() 
{
   switch (mState)
   {
      case Initialized:
         mRegister = std::auto_ptr<SipMessage>(Helper::makeRegister(mAor, mFrom, mContact));
         break;
      default:
         break;
   }
   return *mRegister;
}

SipMessage&
Registration::refreshRegistration() 
{
   //assert(mState == Active);
   mRegister->header(h_CSeq).sequence()++;
   mRegister->header(h_Vias).front().param(p_branch).reset();
   return *mRegister;
}

SipMessage&
Registration::unregister() 
{
   assert(mState == Active);
   mRegister->header(h_CSeq).sequence()++;
   mRegister->header(h_Contacts).clear();
   NameAddr wildcard;
   wildcard.setAllContacts();
   mRegister->header(h_Contacts).push_front(wildcard);
   return *mRegister;
}

void
Registration::handleResponse(const SipMessage& response)
{
   assert(response.isResponse());
   if (response.header(h_StatusLine).statusCode() == 200)
   {
      mState = Active;
      mContacts = response.header(h_Contacts);
      mTimeTillExpiration = response.header(h_Expires).value() * 1000;
   }
   else
   {
      mContacts.clear();
      mState = Terminated;
   }
}

const CallID& 
Registration::getCallID() const
{
   return mRegister->header(h_CallID);
}

bool
Registration::isRegistered() const
{
   return mState == Active;
}

UInt64
Registration::getTimeToRefresh() const
{
   return mTimeTillExpiration;
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
