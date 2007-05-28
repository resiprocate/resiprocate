
#include <iostream>
#include <memory>
#include <list>

#include "rutil/DataStream.hxx"

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ExternalBodyContents.hxx"
#include "TestSupport.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define CRLF "\r\n"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
  Log::initialize(Log::Cout, Log::Info, argv[0]);

  {  
     Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
              "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
              "To: Bob <sip:bob@biloxi.com>\r\n"
              "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
              "Call-ID: a84b4c76e66710\r\n"
              "CSeq: 314159 INVITE\r\n"
              "Max-Forwards: 70\r\n"
              "Contact: <sip:alice@pc33.atlanta.com>\r\n"
              "Content-Type: message/external-body\r\n"
              "Content-Length: 57\r\n"
              "\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: 11\r\n");
     try
     {
        auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
        Contents* body = msg->getContents();

        assert(body != 0);
        ExternalBodyContents* frag = dynamic_cast<ExternalBodyContents*>(body);
        assert(frag != 0);
        assert(frag->message().header(h_ContentType) == Mime("text", "plain"));
        assert(frag->message().header(h_ContentLength).value() == 11);
        assert(!(frag->message().isRequest() ||frag->message().isResponse()));
     }
     catch (BaseException& e)
     {
        assert(false);
     }
  }

  {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: message/sipfrag\r\n"
               "Content-Length: 35\r\n"
               "\r\n"
               "INVITE sip:bob@biloxi.com SIP/2.0\r\n");
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      assert(body);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      assert(frag);
  }
  return 0;
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
