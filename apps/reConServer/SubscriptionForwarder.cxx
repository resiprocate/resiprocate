#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/stack/Uri.hxx>
#include <AppSubsystem.hxx>

#include "SubscriptionForwarder.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace reconserver;

SubscriptionForwarder::SubscriptionForwarder(ConfigParse& configParse, SipStack& stack) :
   mStack(stack),
   mShutdownState(Running)
{
   mStack.registerTransactionUser(*this, true);

   MessageFilterRuleList ruleList;

   // we handle any SUBSCRIBE
   MessageFilterRule::MethodList methodList1;
   methodList1.push_back(resip::SUBSCRIBE);
   ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList1) );

   // we handle NOTIFY for the Event types specified in this rule
   MessageFilterRule::MethodList methodList2;
   methodList2.push_back(resip::NOTIFY);
   MessageFilterRule::EventList eventList;
   eventList.push_back("presence");
   eventList.push_back("message-summary");
   ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList2,
                                           eventList) );

   setMessageFilterRuleList(ruleList);

   mPath = configParse.getConfigData("B2BUARegistrationForwardPath", "");
   const Data& nextHop = configParse.getConfigData("B2BUANextHop", "");
   const Data& subscriptionRoute = configParse.getConfigData("B2BUASubscriptionForwardRoute", nextHop);
   if(subscriptionRoute.empty())
   {
      CritLog(<<"Please specify B2BUASubscriptionForwardRoute");
      throw ConfigParse::Exception("Please specify B2BUASubscriptionForwardRoute", __FILE__, __LINE__);
   }
   mSubscriptionRoute = NameAddr(subscriptionRoute);
   mMissingEventHack = configParse.getConfigBool("B2BUASubscriptionMissingEventHack", false);
   mOverrideContactWithAor = configParse.getConfigBool("B2BUASubscriptionOverrideContactWithAor", false);
   mMaxExpiry = configParse.getConfigInt("B2BUASubscriptionForwardMaxExpiry", 300);

}

SubscriptionForwarder::~SubscriptionForwarder()
{
}

bool
SubscriptionForwarder::process(resip::Lockable* mutex)
{
   if (mFifo.messageAvailable())
   {
      resip::PtrLock lock(mutex);
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   return mFifo.messageAvailable();
}

const Data&
SubscriptionForwarder::name() const
{
   static Data n("SubscriptionForwarder");
   return n;
}

void
SubscriptionForwarder::internalProcess(std::auto_ptr<Message> msg)
{
   // After a Stack ShutdownMessage has been received, don't do anything else in this TU
   if (mShutdownState == Shutdown)
   {
      return;
   }

   {
      TransactionUserMessage* tuMsg = dynamic_cast<TransactionUserMessage*>(msg.get());
      if (tuMsg)
      {
         InfoLog (<< "TU unregistered ");
         resip_assert(mShutdownState == RemovingTransactionUser);
         resip_assert(tuMsg->type() == TransactionUserMessage::TransactionUserRemoved);
         mShutdownState = Shutdown;
         return;
      }
   }

   Data tid = Data::Empty;
   {
      SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
      if (sipMsg)
      {
         tid = sipMsg->getTransactionId();
         bool garbage=false;
         Data reason;

         if(!sipMsg->header(h_From).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed From, ",16);
         }

         if(!sipMsg->header(h_To).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed To, ",14);
         }

         if(!sipMsg->header(h_CallId).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed Call-Id, ",19);
         }

         if(garbage)
         {
            if(sipMsg->isRequest() && sipMsg->method()!=ACK)
            {
               // .bwc. Either we need to trim the last comma off, or make this
               // a proper sentence fragment. This is more fun.
               reason.append("fix your code!",14);
               SipMessage failure;
               //makeResponse(failure, *sipMsg, 400, reason);
               //sendResponse(failure);
            }
            InfoLog (<< "Malformed header in message (" << reason << ") - rejecting/discarding: " << *sipMsg);

            // .bwc. Only forge a response when appropriate, but return in any
            // case.
            return;
         }

         // This should be a SUBSCRIBE or NOTIFY request or response
         // - update contact
         // - add next hop to route set
         // - add via
         // - add record route
         // - forward
         // - reduce hop count
         if(sipMsg->isRequest() && sipMsg->method()==SUBSCRIBE)
         {
            DebugLog(<<"Handling a SUBSCRIBE request");
            sipMsg->header(h_RecordRoutes).push_front(NameAddr(mPath));
            sipMsg->header(h_Routes).clear(); // remove ourselves
            sipMsg->header(h_Routes).push_front(mSubscriptionRoute);
            // some phones send SUBSCRIBE without the Event header, workaround:
            if(!sipMsg->exists(h_Event) && mMissingEventHack)
            {
               sipMsg->header(h_Event).value() = "presence";
            }
            // must clear tlsDomain, otherwise the stack tries to send over TLS
            // even if the route is not a secure transport
            sipMsg->setTlsDomain("");
            Uri& uri = sipMsg->header(h_Contacts).front().uri();
            /*if(uri.exists(p_transport))
            {
               StackLog(<<"removing transport parameter from Contact header");
               uri.remove(p_transport);
            }*/
            // replace Contact header with From header value so the presence
            // server will route the NOTIFY to the registration server which
            // will then set the route correctly.
            if(mOverrideContactWithAor)
            {
               uri = sipMsg->header(h_From).uri();
            }
            if(sipMsg->exists(h_Expires) && sipMsg->header(h_Expires).value() > mMaxExpiry)
            {
               sipMsg->header(h_Expires).value() = mMaxExpiry;
            }
            mStack.send(*sipMsg, this);
         }
         else if(sipMsg->isRequest() && sipMsg->method()==NOTIFY) // FIXME - check event type
         {
            DebugLog(<<"Handling a NOTIFY request");
            //sipMsg->header(h_RecordRoutes).push_front(NameAddr(mPath));
            sipMsg->header(h_Routes).pop_front(); // remove ourselves
            //sipMsg->header(h_Routes).push_front(mSubscriptionRoute);
            // must clear tlsDomain, otherwise the stack tries to send over TLS
            // even if the route is not a secure transport
            //sipMsg->setTlsDomain("");
            mStack.send(*sipMsg, this);
         }
         else if(sipMsg->isResponse() && (sipMsg->header(h_CSeq).method() == SUBSCRIBE ||
            sipMsg->header(h_CSeq).method() == NOTIFY))
         {
            DebugLog(<<"Handling a response");
            //sipMsg->header(h_Vias).pop_front();
            sipMsg->header(h_Vias).front().remove(p_rport); // FIXME - working around rport issue between edge proxy and SBC
            mStack.send(*sipMsg, this);
         }
         else
         {
            WarningLog(<<"Message is not a supported request or response, ignoring it");
            return;
         }
      }

   }
}


/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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

