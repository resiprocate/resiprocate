#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <AppSubsystem.hxx>

#include "RegistrationForwarder.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace reconserver;

RegistrationForwarder::RegistrationForwarder(ConfigParse& configParse, SipStack& stack) :
   mStack(stack),
   mShutdownState(Running)
{
   mStack.registerTransactionUser(*this, true);
   MessageFilterRuleList ruleList;
   MessageFilterRule::MethodList methodList;
   methodList.push_back(resip::REGISTER);
   ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList) );
   setMessageFilterRuleList(ruleList);

   mMaxExpiry = configParse.getConfigInt("B2BUARegistrationForwardMaxExpiry", 60);
   mPath = configParse.getConfigData("B2BUARegistrationForwardPath", "");
   const Data& nextHop = configParse.getConfigData("B2BUANextHop", "");
   const Data& registrationRoute = configParse.getConfigData("B2BUARegistrationForwardRoute", nextHop);
   if(registrationRoute.empty())
   {
      CritLog(<<"Please specify B2BUARegistrationForwardRoute");
      throw ConfigParse::Exception("Please specify B2BUARegistrationForwardRoute", __FILE__, __LINE__);
   }
   mRegistrationRoute = NameAddr(registrationRoute);
}

RegistrationForwarder::~RegistrationForwarder()
{
}

bool
RegistrationForwarder::process(resip::Lockable* mutex)
{
   if (mFifo.messageAvailable())
   {
      resip::PtrLock lock(mutex);
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   return mFifo.messageAvailable();
}

const Data&
RegistrationForwarder::name() const
{
   static Data n("RegistrationForwarder");
   return n;
}

void
RegistrationForwarder::internalProcess(std::auto_ptr<Message> msg)
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

         // This should be a REGISTER request or response
         // - update contact
         // - add next hop to route set
         // - add via
         // - add record route
         // - forward
         // - reduce hop count
         if(sipMsg->isRequest() && sipMsg->method()==REGISTER)
         {
            DebugLog(<<"Handling a REGISTER request");
            if(!sipMsg->header(h_Supporteds).find(Token(Symbols::Path)))
            {
               sipMsg->header(h_Supporteds).push_back(Token(Symbols::Path));
            }
            if(sipMsg->header(h_Expires).value() > mMaxExpiry)
            {
               sipMsg->header(h_Expires).value() = mMaxExpiry;
            }
            if(!mPath.empty())
            {
               sipMsg->header(h_Paths).push_front(NameAddr(mPath));
            }
            sipMsg->header(h_Routes).clear(); // remove ourselves
            sipMsg->header(h_Routes).push_front(mRegistrationRoute);
            // must clear tlsDomain, otherwise the stack tries to send over TLS
            // even if the route is not a secure transport
            sipMsg->setTlsDomain("");
            mStack.send(*sipMsg, this);
         }
         else if(sipMsg->isResponse() && sipMsg->header(h_CSeq).method() == REGISTER)
         {
            DebugLog(<<"Handling a REGISTER response");
            mStack.send(*sipMsg, this);
         }
         else
         {
            WarningLog(<<"Message is not a REGISTER request or response, ignoring it");
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

