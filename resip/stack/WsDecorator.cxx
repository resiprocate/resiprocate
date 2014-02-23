#include "WsDecorator.hxx"

#include "SipMessage.hxx"
#include "Tuple.hxx"
#include "Transport.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace std;
using namespace resip;

WsDecorator::WsDecorator()
{
}

WsDecorator::~WsDecorator()
{
}

void 
WsDecorator::decorateMessage(resip::SipMessage &msg, const resip::Tuple &source, const resip::Tuple &destination, const resip::Data& sigcompId)
{
   const resip::Tuple& wsSource = msg.getSource();

   if(wsSource.getType() == resip::WS) 
   {
      if(msg.exists(resip::h_Contacts)) 
      {
         resip::NameAddr& contact = msg.header(resip::h_Contacts).front();

         if (isEqualNoCase(contact.uri().host(), resip::Data("df7jal23ls0d.invalid"))) 
         {
            contact.uri().host() = resip::Tuple::inet_ntop(source);
            contact.uri().port() = source.getPort();
            contact.uri().param(resip::p_transport) = resip::Tuple::toDataLower(source.getType());

            contact.uri().param(resip::p_wsSrcIp) = resip::Tuple::inet_ntop(wsSource);
            contact.uri().param(resip::p_wsSrcPort) = wsSource.getPort();
         }
      }

      if(msg.exists(resip::h_Vias)) 
      {
         resip::Via &via = msg.header(resip::h_Vias).back();
         if(isEqualNoCase(via.sentHost(), resip::Data("df7jal23ls0d.invalid"))) 
         {
            via.sentHost() = resip::Tuple::inet_ntop(wsSource);
            via.sentPort() = wsSource.getPort();
            via.transport() = "TCP"; // most servers get crazy if we send "WS" as transport
         }
      }
   }
}

void 
WsDecorator::rollbackMessage(resip::SipMessage& msg)
{
}

MessageDecorator* WsDecorator::clone() const
{
   return new WsDecorator(*this);
}

/* ====================================================================
 *
 * Copyright 2012 Doubango Telecom.  All rights reserved.
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

