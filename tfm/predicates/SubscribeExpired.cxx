#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/predicates/SubscribeExpired.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST
using namespace resip;

SubscribeExpired::SubscribeExpired()
{}

bool 
SubscribeExpired::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<resip::SipMessage> msg = sipEvent->getMessage();
   
   return (*this)(msg);
}

bool
SubscribeExpired::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   if (!msg->exists(h_SubscriptionState))
   {
      ErrLog(<< "NOTIFY without a Subscription-State header");
      return false;
   }

   if (msg->header(h_SubscriptionState).value() != "terminated")
   {
      InfoLog(<< "SubscribeExpired failed: Subscription-State: terminated");
      return false;
   }
   
   return msg->header(h_SubscriptionState).param(p_reason) == "timeout";
}

/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
