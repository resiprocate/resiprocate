#include <memory>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"
#include "resiprocate/Dialog.hxx"

#include "InviteServer.hxx"
#include "Transceiver.hxx"

using namespace resip;
using namespace Loadgen;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

InviteServer::InviteServer(Transceiver& transceiver)
   : mTransceiver(transceiver)
{}

void
InviteServer::go()
{
   NameAddr contact;
   contact.uri() = mTransceiver.contactUri();
   while (true)
   {
      try
      {
         auto_ptr<SipMessage> invite(waitForRequest(INVITE, 100000));
         contact.uri().user() = invite->header(h_RequestLine).uri().user();
         
         auto_ptr<SipMessage> i_100(Helper::makeResponse(*invite, 100, "Trying"));
         mTransceiver.send(*i_100);

         Data localTag = Helper::computeTag(4);

         auto_ptr<SipMessage> i_180(Helper::makeResponse(*invite, 180, contact, "Ringing"));
         i_180->header(h_To).uri().param(p_tag) = localTag;
         DebugLog(<< "constructed 180: " << *i_180);
         mTransceiver.send(*i_180);

         auto_ptr<SipMessage> i_200(Helper::makeResponse(*invite, 200, contact, "OK"));
         i_200->header(h_To).uri().param(p_tag) = localTag;
         mTransceiver.send(*i_200);
         
         auto_ptr<SipMessage> ack(waitForRequest(ACK, 1000));
         auto_ptr<SipMessage> bye(waitForRequest(BYE, 1000));

         auto_ptr<SipMessage> b_200(Helper::makeResponse(*bye, 200, contact, "OK"));
         mTransceiver.send(*b_200);
      }
      catch(Exception e)
      {
         ErrLog(<< "Proxy not responding.");
         exit(-1);
      }
   }
}

SipMessage* 
InviteServer::waitForResponse(int responseCode,
                              int waitMs)
{
   SipMessage* reg = mTransceiver.receive(waitMs);
   if(reg)
   {         
      if (reg->isResponse() &&
          reg->header(h_StatusLine).responseCode() == responseCode)
      {
         return reg;
      }
      else
      {
         throw Exception("Invalid response.", __FILE__, __LINE__);
      }
   }
   else
   {
      throw Exception("Timed out.", __FILE__, __LINE__);
   }
}

SipMessage* 
InviteServer::waitForRequest(MethodTypes method,
                             int waitMs)
{
   SipMessage* req = mTransceiver.receive(waitMs);
   if(req)
   {         
      if (req->isRequest() &&
          req->header(h_RequestLine).getMethod() == method)
      {
         return req;
      }
      else
      {
         throw Exception("Invalid request.", __FILE__, __LINE__);
      }
   }
   else
   {
      throw Exception("Timed out.", __FILE__, __LINE__);
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
