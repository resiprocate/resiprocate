#include <cassert>
#include <fstream>
#include <ostream>

#include "rutil/Logger.hxx"
#include "resip/stack/Security.hxx"

#include "TestSupport.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Debug, Data::Empty);

#ifdef USE_SSL
   Security* security=0;
   try
   {
      security = new Security;
   }
   catch( ... )
   {
      security = 0;
      ErrLog( << "Got a exception setting up Security" );
      return -1;
   }

   try
   {
      assert(security != 0);
      security->preload();
   }
   catch( ... )
   {
      ErrLog( << "Got a exception loading certificates" );
      return -1;
   }

   assert( security );

   {
        ErrLog( << "\n\nStarting test one" );

      Data txt1 = 
         "INVITE sip:bob@biloxi.exmple.org SIP/2.0\r\n"
         "Via: SIP/2.0/TLS pc33.atlanta.example.com;branch=z9hG4bKnashds8\r\n"
         "To: Bob <sip:bob@biloxi.example.org>\r\n"
        "From: Alice <sip:alice@atlanta.example.com>;tag=1928301774\r\n"
        "Call-ID: a84b4c76e66710\r\n"
        "CSeq: 314159 INVITE\r\n"
        "Max-Forwards: 70\r\n"
        "Date: Thu, 21 Feb 2002 13:02:03 GMT\r\n"
        "Contact: <sip:alice@pc33.atlanta.example.com>\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: 147\r\n"
        "\r\n"
        "v=0\r\n"
        "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
        "s=Session SDP\r\n"
        "c=IN IP4 pc33.atlanta.example.com\r\n"
        "t=0 0\r\n"
        "m=audio 49172 RTP/AVP 0\r\n"
        "a=rtpmap:0 PCMU/8000\r\n";
     
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt1));

      try
      {
         const Data& domain = msg->header(h_From).uri().host();
         msg->header(h_Identity).value() = security->computeIdentity( domain,
                                                                       msg->getCanonicalIdentityString());
      }
      catch (Security::Exception& e)
      {
         ErrLog (<< "Couldn't add identity header: " << e);
         msg->remove(h_Identity);
      }
      
      ErrLog( << "INVITE base64 identity is " <<  msg->header(h_Identity).value() );
   }

   {
        ErrLog( << "\n\nStarting test two" );

      Data txt2 = 
         "BYE sip:alice@pc33.atlanta.example.com SIP/2.0\r\n"
         "Via: SIP/2.0/TLS 192.0.2.4;branch=z9hG4bKnashds10\r\n"
         "Max-Forwards: 70\r\n"
         "From: Bob <sip:bob@biloxi.example.org>;tag=a6c85cf\r\n"
         "To: Alice <sip:alice@atlanta.example.com>;tag=1928301774\r\n"
         "Date: Thu, 21 Feb 2002 14:19:51 GMT\r\n"
         "Call-ID: a84b4c76e66710\r\n"
         "CSeq: 231 BYE\r\n"
         "Content-Length: 0\r\n"
         "\r\n"
         ;
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt2));
      
      try
      {
         const Data& domain = msg->header(h_From).uri().host();

         Data identString = msg->getCanonicalIdentityString();
         
         msg->header(h_Identity).value() = security->computeIdentity( domain, identString );
      }
      catch (Security::Exception& e)
      {
         ErrLog (<< "Couldn't add identity header: " << e);
         msg->remove(h_Identity);
      }
      
      ErrLog( << "BYE base64 identity is " <<  msg->header(h_Identity).value() );
}



   {
        ErrLog( << "\n\nStarting test three - conect iden - invite(2) " );

      Data txt1 =

"INVITE sip:Carol@ua2.example.com SIP/2.0\r\n"
"Via: SIP/2.0/TLS proxy.example.com;branch=z9hG4bK776asdhds\r\n"
"Via: SIP/2.0/TLS ua1.example.com;branch=z9hG4bKnashds8;received=192.0.2.1\r\n"
"To: Bob <sip:bob@example.com>\r\n"
"From: Alice <sip:alice@example.com>;tag=13adc987\r\n"
"Call-ID: 12345600@ua1.example.com\r\n"
"CSeq: 1 INVITE\r\n"
"Max-Forwards: 69\r\n"
"Date: Thu, 21 Feb 2002 13:02:03 GMT\r\n"
"Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, UPDATE\r\n"
"Supported: from-change\r\n"
"Contact: <sip:alice@ua1.example.com>\r\n"
"Record-Route: <sip:proxy.example.com;lr>\r\n"
"Identity-Info: <https://example.com/example.cer>;alg=rsa-sha1\r\n"
"Content-Type: application/sdp\r\n"
"Content-Length: 154\r\n"
"\r\n"
"v=0\r\n"
"o=UserA 2890844526 2890844526 IN IP4 ua1.example.com\r\n"
"s=Session SDP\r\n"
"c=IN IP4 ua1.example.com\r\n"
"t=0 0\r\n"
"m=audio 49172 RTP/AVP 0\r\n"
"a=rtpmap:0 PCMU/8000\r\n"
         ;
      
     
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt1));

      try
      {
         const Data& domain = msg->header(h_From).uri().host();
         msg->header(h_Identity).value() = security->computeIdentity( domain,
                                                                       msg->getCanonicalIdentityString());
      }
      catch (Security::Exception& e)
      {
         ErrLog (<< "Couldn't add identity header: " << e);
         msg->remove(h_Identity);
      }
      
      ErrLog( << "base64 identity is " <<  msg->header(h_Identity).value() );
   }




   {
        ErrLog( << "\n\nStarting test four - connected-id update(8) " );

      Data txt2 =

"UPDATE sip:Alice@ua1.example.com SIP/2.0\r\n"
"Via: SIP/2.0/TLS proxy.example.com;branch=z9hG4bK776asdhdu\r\n"
"Via: SIP/2.0/TLS ua2.example.com;branch=z9hG4bKnashdt1;received=192.0.2.3\r\n"
"From: Carol <sip:Carol@example.com>;tag=2ge46ab5\r\n"
"To: Alice <sip:Alice@example.com>;tag=13adc987\r\n"
"Call-ID: 12345600@ua1.example.com\r\n"
"CSeq: 2 UPDATE\r\n"
"Max-Forwards: 69\r\n"
"Date: Thu, 21 Feb 2002 13:02:15 GMT\r\n"
"Contact: <sip:Carol@ua2.example.com>\r\n"
"Identity-Info: <https://example.com/cert>;alg=rsa-sha1\r\n"
"Content-Length: 0\r\n"
"\r\n"
         ;
      
            
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt2));
      
      try
      {
         const Data& domain = msg->header(h_From).uri().host();

         Data identString = msg->getCanonicalIdentityString();
         
         msg->header(h_Identity).value() = security->computeIdentity( domain, identString );
      }
      catch (Security::Exception& e)
      {
         ErrLog (<< "Couldn't add identity header: " << e);
         msg->remove(h_Identity);
      }
      
      ErrLog( << "base64 identity is " <<  msg->header(h_Identity).value() );
}




   {
        ErrLog( << "\n\nStarting test five - connected-id reinvite 6  " );

      Data txt2 = 

"INVITE sip:alice@ua1.example.com SIP/2.0\r\n"
"Via: SIP/2.0/TLS b2bua.example.com;branch=z9hG4bKnashdxy\r\n"
"From: Carol <sip:Carol@example.com>;tag=2ge46ab5\r\n"
"To: Alice <sip:Alice@example.com>;tag=13adc987\r\n"
"Call-ID: 12345600@ua1.example.com\r\n"
"CSeq: 3 INVITE\r\n"
"Max-Forwards: 70\r\n"
"Date: Thu, 21 Feb 2002 13:03:20 GMT\r\n"
"Contact: <sip:xyz@b2bua.example.com>\r\n"
"Identity-Info: <https://example.com/cert>;alg=rsa-sha1\r\n"
"Content-Length: 0\r\n"
         "\r\n";
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt2));
      
      try
      {
         const Data& domain = msg->header(h_From).uri().host();

         Data identString = msg->getCanonicalIdentityString();
         
         msg->header(h_Identity).value() = security->computeIdentity( domain, identString );
      }
      catch (Security::Exception& e)
      {
         ErrLog (<< "Couldn't add identity header: " << e);
         msg->remove(h_Identity);
      }
       ErrLog( << "base64 identity is " <<  msg->header(h_Identity).value() );
}     
   





#endif // use_ssl 

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
