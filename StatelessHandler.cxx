#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DnsUtil.hxx"

#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/DnsResolver.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/StatelessHandler.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/TransportMessage.hxx"



using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatelessHandler::StatelessHandler(TransactionController& c) : mController(c)
{
}

void 
StatelessHandler::process()
{
   Message* msg = mController.mStateMacFifo.getNext();
   assert(msg);

   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   TransportMessage* transport = dynamic_cast<TransportMessage*>(msg);
   
   if (sip)
   {
      if (sip->header(h_Vias).empty())
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
            via.param(p_rport).port() = sip->getSource().port;
            mController.mTUFifo.add(sip);
         }
         else if (sip->isRequest())
         {
            if (sip->getDestination().transport)
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               mController.mTransportSelector.transmit(sip, sip->getDestination()); // results not used
            }
            else
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               StatelessMessage* stateless = new StatelessMessage(mController.mTransportSelector, sip);
               mController.mTransportSelector.dnsResolve(sip, stateless);
            }
         }
         else // no dns for sip responses
         {
            assert(sip->isResponse());
            DebugLog (<< "Processing response from TU: " << msg->brief());
            const Via& via = sip->header(h_Vias).front();

            Tuple destination;
            DnsUtil::inet_pton(via.param(p_received), destination.ipv4);
			if (via.exists(p_rport) && via.param(p_rport).hasValue())
            {
               destination.port = via.param(p_rport).port();
            }
            else
            {
               destination.port = via.sentPort();
            }
            destination.transportType = Tuple::toTransport(via.transport());
            
            mController.mTransportSelector.transmit(sip, destination); // results not used
         }
      }
   }
   else if (transport)
   {
      DebugLog (<< "Processing Transport result: " << msg->brief());
      
      if (transport->isFailed())
      {
         InfoLog (<< "Not yet supported");
      }
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
