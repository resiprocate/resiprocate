
#include <iostream>
#include <memory>
#include <list>

#include "rutil/DataStream.hxx"

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/Pidf.hxx"
#include "TestSupport.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/test/tassert.h"

using namespace resip;
using namespace std;

#define CRLF "\r\n"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
  tassert_init(5);

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
        "Content-Type: message/sipfrag\r\n"
        "Content-Length: 59\r\n"
        "\r\n" 
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "\r\n" 
        "Hi There!\r\n");

     try
     {
        auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
        Contents* body = msg->getContents();

        assert(body != 0);
        SipFrag* frag = dynamic_cast<SipFrag*>(body);
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

     // test from RFC3420
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
                 "Content-Length: 38\r\n"
                 "\r\n"
                 "INVITE sip:alice@atlanta.com SIP/2.0\r\n");
        
        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
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
                 "Content-Length: 22\r\n"
                 "\r\n"
                 "SIP/2.0 603 Declined\r\n");

        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
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
                 "Content-Length: 150\r\n"
                 "\r\n"
                 "REGISTER sip:atlanta.com SIP/2.0\r\n"
                 "To: sip:alice@atlanta.com\r\n"
                 "Contact: <sip:alicepc@atlanta.com>;q=0.9,\r\n"
                 "         <sip:alicemobile@atlanta.com>;q=0.1\r\n");

        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
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
                 "Content-Length: 89\r\n"
                 "\r\n"
                 "SIP/2.0 400 Bad Request\r\n"
                 "Warning: 399 atlanta.com Your Event header field was malformed\r\n");
        
        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
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
                 "Content-Length: 259\r\n"
                 "\r\n"
                 "From: Alice <sip:alice@atlanta.com>\r\n"
                 "To: Bob <sip:bob@biloxi.com>\r\n"
                 "Contact: <sip:alice@pc33.atlanta.com>>href=mailto:<sip:alice@pc33.atlanta.com>><sip:alice@pc33.atlanta.com>\r\n"
                 "Date: Thu, 21 Feb 2002 13:02:03 GMT\r\n"
                 "Call-ID: a84b4c76e66710\r\n"
                 "Cseq: 314159 INVITE\r\n");

        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
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
                 "Content-Length: 150\r\n"
                 "\r\n"
                 "SIP/2.0 200 OK\r\n"
                 "Content-Type: application/sdp\r\n"
                 "Content-Length: 314\r\n"
                 "\r\n"
                 "v=0\r\n"
                 "o=alice 2890844526 2890844526 IN IP4 host.anywhere.com\r\n"
                 "s=\r\n"
                 "c=IN IP4 host.anywhere.com\r\n"
                 "t=0 0\r\n"
                 "m=audio 49170 RTP/AVP 0\r\n"
                 "a=rtpmap:0 PCMU/8000\r\n"
                 "m=video 51372 RTP/AVP 31\r\n"
                 "a=rtpmap:31 H261/90000\r\n"
                 "m=video 53000 RTP/AVP 32\r\n"
                 "a=rtpmap:32 MPV/90000");
        try
        {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));        
        }
        catch (BaseException& e)
        {
           assert(false);
        }
     }
  

  {
     // !ah! This test doesn't work with the MsgHeaderScanner -- works w/ preparser.

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
      std::cout << "hiya " << std::endl;
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      tassert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      tassert(frag != 0);

      cout << "!! ";
      frag->encode(resipCout);

      tassert(frag->message().header(h_RequestLine).uri().user() == "bob");
      msg->encode(resipCout);
      tassert_verify(1);
   }

   {
     tassert_reset();
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
               "SIP/2.0 503 Service Unavailable" CRLF " " 

               // space causes MsgHeaderScanner to claim not at end of message
               // .dlb. this is correct: message-header is terminated with CRLF.

               CRLF);
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      tassert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      tassert(frag != 0);

      tassert(frag->message().header(h_StatusLine).responseCode() == 503);
      msg->encode(resipCout);
      tassert_verify(2);
   }

   {
     tassert_reset();
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
               "Cseq: 314159 INVITE");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      tassert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      tassert(frag != 0);

      cout << "!! ";
      frag->encode(resipCout);

      tassert(frag->message().exists(h_From));
      tassert(frag->message().header(h_From).uri().user() == "alice");

      tassert(frag->message().exists(h_CSeq));
      tassert(frag->message().header(h_CSeq).sequence() == 314159);
      
      msg->encode(resipCout);
      tassert_verify(3);
   }
   {
     tassert_reset();
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
               "INVITE sip:bob@biloxi.com SIP/2.0"
        );

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      Contents* body = msg->getContents();

      tassert(body != 0);
      SipFrag* frag = dynamic_cast<SipFrag*>(body);
      tassert(frag != 0);

      cout << "!! ";
      frag->encode(resipCout);

      tassert(frag->message().header(h_RequestLine).getMethod() == INVITE);
      tassert(frag->message().header(h_RequestLine).getSipVersion() == "SIP/2.0");
      
      msg->encode(resipCout);

      Contents* body2 = msg->getContents();
      tassert(body2);
      SipFrag* f2 = dynamic_cast<SipFrag*>(body);
      tassert(f2);

      {
        Data d;
        DataStream ds(d);
        f2->encode(ds);
        ds.flush();

        cout << "SipFrag without contents encoded:" << endl;
        f2->encode(resipCout);


        tassert(d.find("Content-Length") == Data::npos);

      }
      tassert_verify(4);
   }

   {
      Data contents(
               "--ncpq7w94hfb\r\n"
               "Content-ID: <foo@example.com>\r\n"
               "Content-Type: message/sipfrag\r\n"
               "Content-Transfer-Encoding: binary\r\n"
               "\r\n"
               "SIP/2.0 200 OK\r\n"
               "Via: SIP/2.0/UDP pc3.atlanta.com;branch=z9hG4bKmc0q\r\n"
               "To: Joe <sip:joe@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=87125645\r\n"
               "Call-ID: nbc9w84wbcalkj\r\n"
               "CSeq: -87235\r\n"
               "--ncpq7w94hfb\r\n"
               "Content-Transfer-Encoding: binary\r\n"
               "Content-ID: <bUZBsM@pres.example.com>\r\n"
               "Content-Type: application/pidf+xml;charset=\"UTF-8\"\r\n"
               "\r\n"
               "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
               "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
               "    entity=\"sip:bob@example.com\">\r\n"
               "  <tuple id=\"sg89ae\">\r\n"
               "    <status>\r\n"
               "      <basic>open</basic>\r\n"
               "    </status>\r\n"
               "    <contact priority=\"1.0\">sip:bob@example.com</contact>\r\n"
               "  </tuple>\r\n"
               "</presence>\r\n"
               "\r\n"
               "--ncpq7w94hfb--\r\n");

      Data msg;
      msg += Data()+"INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Content-Type: multipart/mixed;type=\"message/sipfrag\";\r\n"
               "    start=\"<foo@example.com>\";boundary=\"ncpq7w94hfb\"\r\n"
               "Content-Length: " + Data(contents.size()) + "\r\n"
               "\r\n" + contents;
               
      auto_ptr<SipMessage> message(TestSupport::makeMessage(msg.c_str()));
      
      SipMessage* copy=0;
      {
         Contents* body = message->getContents();

         tassert(body != 0);
         MultipartMixedContents* mmixed = dynamic_cast<MultipartMixedContents*>(body);
         tassert(mmixed != 0);


         cout << "!! ";
         mmixed->encode(resipCout);

         MultipartMixedContents::Parts& parts=mmixed->parts();

         copy = new SipMessage(*message);

         tassert(parts.size()==2);

         SipFrag* frag=dynamic_cast<SipFrag*>(parts.front());
         tassert(frag);
         if(frag)
         {
            frag->encode(resipCout);
         }
         
         Pidf* pidf=dynamic_cast<Pidf*>(parts.back());
         tassert(pidf);
         if(pidf)
         {
            pidf->encode(resipCout);
         }

         message->encode(resipCout);
      }

      {
         Contents* body = copy->getContents();

         tassert(body != 0);
         MultipartMixedContents* mmixed = dynamic_cast<MultipartMixedContents*>(body);
         tassert(mmixed != 0);


         cout << "!! ";
         mmixed->encode(resipCout);

         MultipartMixedContents::Parts& parts=mmixed->parts();


         tassert(parts.size()==2);

         SipFrag* frag=dynamic_cast<SipFrag*>(parts.front());
         tassert(frag);
         if(frag)
         {
            frag->encode(resipCout);
         }
         
         Pidf* pidf=dynamic_cast<Pidf*>(parts.back());
         tassert(pidf);
         if(pidf)
         {
            pidf->encode(resipCout);
         }

         copy->encode(resipCout);
      }

      delete copy;

      tassert_verify(5);
      

   }

   tassert_report();

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
