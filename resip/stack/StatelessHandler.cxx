#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#include <svcguid.h>
#ifdef USE_IPV6
#include <ws2tcpip.h>
#endif
#endif

#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"

#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/StatelessHandler.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "rutil/WinLeakCheck.hxx"



using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatelessHandler::StatelessHandler(TransactionController& c) : mController(c)
{
}

void 
StatelessHandler::process()
{
   Message* msg = mController.mStateMacFifo.getNext();
   resip_assert(msg);

   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   TransportFailure* transport = dynamic_cast<TransportFailure*>(msg);
   
   if (sip)
   {
      if (sip->const_header(h_Vias).empty())
      {
         InfoLog(<< "TransactionState::process dropping message with no Via: " << sip->brief());
         delete sip;
         return;
      }
      else
      {
         if (sip->isExternal())
         {
            DebugLog (<< "Processing sip from wire: " << msg->brief());
            Via& via = sip->header(h_Vias).front();
            // this is here so that we will reuse the tcp connection
            via.param(p_rport).port() = sip->getSource().getPort();
            mController.mTuSelector.add(sip, TimeLimitFifo<Message>::InternalElement);            
         }
         else if (sip->isRequest())
         {
            if (sip->getDestination().mFlowKey)
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               mController.mTransportSelector.transmit(sip, sip->getDestination()); // results not used
            }
            else
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               StatelessMessage* stateless = new StatelessMessage(mController.mTransportSelector, sip);
               DnsResult* dnsRes = mController.mTransportSelector.createDnsResult(stateless);      
               mController.mTransportSelector.dnsResolve(dnsRes, sip);
            }
         }
         else // no dns for sip responses
         {
            resip_assert(sip->isResponse());
            DebugLog (<< "Processing response from TU: " << msg->brief());
            const Via& via = sip->const_header(h_Vias).front();
            int port = via.sentPort();
            if (sip->hasForceTarget())
            {
               resip_assert( /*Unimplemented*/ 0 );
            }
            else
            {
                if (via.exists(p_rport) && via.param(p_rport).hasValue())
                {
                    port = via.param(p_rport).port();
                }
                Tuple destination(via.param(p_received), port, Tuple::toTransport(via.transport()));
                mController.mTransportSelector.transmit(sip, destination); // results not used
            }
         }
      }
   }
   else if (transport)
   {
      DebugLog (<< "Processing Transport result: " << msg->brief());
      InfoLog (<< "Not yet supported");
   }
   else
   {
      DebugLog (<< "Dropping: " << msg->brief());
   }
}


StatelessMessage::StatelessMessage(TransportSelector& selector, SipMessage* msg) : mSelector(selector), mMsg(msg)
{
}

void 
StatelessMessage::rewriteRequest(const Uri& rewrite)
{
   resip_assert(mMsg->isRequest());
   if (mMsg->const_header(h_RequestLine).uri() != rewrite)
   {
      InfoLog (<< "Rewriting request-uri to " << rewrite);
      mMsg->header(h_RequestLine).uri() = rewrite;
   }
}


void 
StatelessMessage::handle(DnsResult* result)
{
   if (result->available() == DnsResult::Available)
   {
      Tuple next = result->next();
      mSelector.transmit(mMsg, next);
   }

   delete this;
   result->destroy();
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
