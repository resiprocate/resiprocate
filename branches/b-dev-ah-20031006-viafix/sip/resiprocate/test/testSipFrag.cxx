#include "resiprocate/os/DataStream.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/SipFrag.hxx"
#include "TestSupport.hxx"
#include "resiprocate/os/Logger.hxx"

#include <iostream>
#include <memory>

using namespace resip;
using namespace std;

#define CRLF "\r\n"

int
main(int argc, char* argv[])
{
    Log::initialize(Log::COUT, Log::INFO, argv[0]);

   {
      // tests end of message problem (MsgHeaderScanner?)
      Data txt("NOTIFY sip:proxy@66.7.238.210:5060 SIP/2.0" CRLF
               "Via: SIP/2.0/UDP  66.7.238.211:5060" CRLF
               "From: <sip:4155540578@66.7.238.211>;tag=525CEC-12FE" CRLF
               "To: <sip:4082401918@larry.gloo.net>;tag=1fd9ba58" CRLF
               "Date: Tue, 23 Sep 2003 05:18:21 GMT" CRLF
               "Call-ID: 2A599BF9-ECBC11D7-8026C991-401DDB71@66.7.238.211" CRLF
               "User-Agent: Cisco-SIPGateway/IOS-12.x" CRLF
               "Max-Forwards: 6" CRLF
               "Route: <sip:AUTO@larry.gloo.net:5200>" CRLF
               "Timestamp: 1064294301" CRLF
               "CSeq: 102 NOTIFY" CRLF
               "Event: refer" CRLF
               "Contact: <sip:4155540578@66.7.238.211:5060>" CRLF
               "Content-Length: 35" CRLF
               "Content-Type: message/sipfrag" CRLF
               CRLF
               "SIP/2.0 503 Service Unavailable" CRLF " " // space causes MsgHeaderScanner to claim not at end of message
               CRLF);
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      assert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      assert(frag != 0);

      assert(frag->message().header(h_StatusLine).responseCode() == 503);
      msg->encode(cerr);

      return 0;
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

      assert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      assert(frag != 0);

      cerr << "!! ";
      frag->encode(cerr);

      assert(frag->message().header(h_RequestLine).uri().user() == "bob");
      msg->encode(cerr);
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
               "\r\n"
               "INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "From: Alice <sip:alice@atlanta.com>\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Date: Thu, 21 Feb 2002 13:02:03 GMT\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "Cseq: 314159 INVITE\r\n\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      assert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      assert(frag != 0);

      cerr << "!! ";
      frag->encode(cerr);

      assert(frag->message().exists(h_From));
      assert(frag->message().header(h_From).uri().user() == "alice");

      assert(frag->message().exists(h_CSeq));
      assert(frag->message().header(h_CSeq).sequence() == 314159);
      
      msg->encode(cerr);
   }

   cerr << "\nTEST OK" << endl;
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
