#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/sipstack/Preparse.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/sipstack/test/TestSupport.hxx"

#include <iostream>
#include <memory>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      char *txt1 = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      auto_ptr<SipMessage> message1(TestSupport::makeMessage(Data(txt1)));
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(Data(txt1)));

      NameAddr local;
      
      local = message1->header(h_From);
      local = message2->header(h_From);
   }

   {
      cerr << "Testing raw header transfer" << endl;
      
      char *txt1 = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      char *txt2 = "REGISTER sip:registrar.ixolib.com SIP/2.0\r\nVia: SIP/2.0/UDP qoqspc.ixolib.com:5060;branch=2222222222\r\nMax-Forwards: 70\r\nTo: Qoq <sip:qoq@ixolib.com>\r\nFrom: Qoq <sip:qoq@ixolib.com>;tag=456248\r\nCall-ID: 111111111111111\r\nCSeq: 6281 REGISTER\r\nContact: <sip:qoq@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      auto_ptr<SipMessage> message1(TestSupport::makeMessage(Data(txt1)));
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(Data(txt2)));
      
      assert(message1->getRawHeader(Headers::CSeq)->getParserContainer() == 0);
      assert(message2->getRawHeader(Headers::CSeq)->getParserContainer() == 0);

      message1->setRawHeader(message2->getRawHeader(Headers::CSeq), Headers::CSeq);
      message1->encode(cerr) << endl;
      assert(message1->header(h_CSeq).sequence() == 6281);
   }

   {
      cerr << "Testing raw header transfer post parse" << endl;
      
      char *txt1 = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      char *txt2 = "REGISTER sip:registrar.ixolib.com SIP/2.0\r\nVia: SIP/2.0/UDP qoqspc.ixolib.com:5060;branch=2222222222\r\nMax-Forwards: 70\r\nTo: Qoq <sip:qoq@ixolib.com>\r\nFrom: Qoq <sip:qoq@ixolib.com>;tag=456248\r\nCall-ID: 111111111111111\r\nCSeq: 6281 REGISTER\r\nContact: <sip:qoq@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";


      auto_ptr<SipMessage> message1(TestSupport::makeMessage(Data(txt1)));
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(Data(txt2)));

      assert(message1->getRawHeader(Headers::CSeq)->getParserContainer() == 0);
      assert(message2->getRawHeader(Headers::CSeq)->getParserContainer() == 0);

      // causes parse
      assert(message2->header(h_CSeq).sequence() == 6281);
      // should have a parsed header
      assert(message2->getRawHeader(Headers::CSeq)->getParserContainer());
      
      // should still work, but copies
      message1->setRawHeader(message2->getRawHeader(Headers::CSeq), Headers::CSeq);
      message1->encode(cerr) << endl;
      assert(message1->header(h_CSeq).sequence() == 6281);

      // should have a parsed header
      assert(message1->getRawHeader(Headers::CSeq)->getParserContainer());
   }

   cout << "All OK" << endl;
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
