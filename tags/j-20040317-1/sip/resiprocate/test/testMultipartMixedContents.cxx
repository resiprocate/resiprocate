#include <iostream>
#include <memory>

#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/PlainContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

using namespace resip;
using namespace std;

int
main()
{
   {
      const Data txt("--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-ID: <1_950120.aaCC@XIson.com>\r\n"
                     "\r\n"
                     "25\r\n"
                     "10\r\n"
                     "34\r\n"
                     "10\r\n"
                     "25\r\n"
                     "21\r\n"
                     "26\r\n"
                     "10\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Description: The fixed length records\r\n"
                     "Content-Transfer-Encoding: base64\r\n"
                     "Content-ID: <2_950120.aaCB@XIson.com>\r\n"
                     "\r\n"
                     "T2xkIE1hY0RvbmFsZCBoYWQgYSBmYXJtCkUgSS\r\n"
                     "BFIEkgTwpBbmQgb24gaGlzIGZhcm0gaGUgaGFk\r\n"
                     "IHNvbWUgZHVja3MKRSBJIEUgSSBPCldpdGggYS\r\n"
                     "BxdWFjayBxdWFjayBoZXJlLAphIHF1YWNrIHF1\r\n"
                     "YWNrIHRoZXJlLApldmVyeSB3aGVyZSBhIHF1YW\r\n"
                     "NrIHF1YWNrCkUgSSBFIEkgTwo=\r\n"
                     "\r\n"
                     "--example-1--");

      // "Content-Type: "
      const Data contentsTxt = ("Multipart/Related; boundary=example-1\r\n"
                                "        start=\"<950120.aaCC@XIson.com>\";\r\n"
                                "        type=\"Application/X-FixedRecord\"\r\n"
                                "     start-info=\"-o ps\"\r\n");

      ParseBuffer pb(contentsTxt.data(), contentsTxt.size());
      Mime contentType;
      contentType.parse(pb);

      HeaderFieldValue hfv(txt.data(), txt.size());
      MultipartRelatedContents mpc(&hfv, contentType);

      assert(mpc.parts().size() == 2);

      PlainContents *f = dynamic_cast<PlainContents*>(mpc.parts().front());
      assert(f);
      f->getBodyData();

      mpc.encode(cerr);
   }

   {
      const Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                     "To: <sip:bob@biloxi.com>\r\n"
                     "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                     "Call-ID: 314159\r\n"
                     "CSeq: 14 INVITE\r\n"
                     "Content-Type: Multipart/Related; boundary=example-1;"
                     "start=\"<950120.aaCC@XIson.com>\";"
                     "type=\"Application/X-FixedRecord\";start-info=\"-o ps\"\r\n"
                     "\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-ID: <1_950120.aaCC@XIson.com>\r\n"
                     "\r\n"
                     "25\r\n"
                     "10\r\n"
                     "34\r\n"
                     "10\r\n"
                     "25\r\n"
                     "21\r\n"
                     "26\r\n"
                     "10\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Description: The fixed length records\r\n"
                     "Content-Transfer-Encoding: base64\r\n"
                     "Content-ID: <2_950120.aaCB@XIson.com>\r\n"
                     "\r\n"
                     "T2xkIE1hY0RvbmFsZCBoYWQgYSBmYXJtCkUgSS\r\n"
                     "BFIEkgTwpBbmQgb24gaGlzIGZhcm0gaGUgaGFk\r\n"
                     "IHNvbWUgZHVja3MKRSBJIEUgSSBPCldpdGggYS\r\n"
                     "BxdWFjayBxdWFjayBoZXJlLAphIHF1YWNrIHF1\r\n"
                     "YWNrIHRoZXJlLApldmVyeSB3aGVyZSBhIHF1YW\r\n"
                     "NrIHF1YWNrCkUgSSBFIEkgTwo=\r\n"
                     "\r\n"
                     "--example-1--");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
      assert(mpc);
      assert(mpc->parts().size() == 2);
      PlainContents *f11 = dynamic_cast<PlainContents*>(mpc->parts().front());
      assert(f11);
      f11->getBodyData();
      MultipartRelatedContents::Parts::const_iterator i = ++mpc->parts().begin();
      PlainContents *f12 = dynamic_cast<PlainContents*>(*i);
      assert(f12);
      f12->getBodyData();

      Data buff;
      {
         DataStream str(buff);
         str << *msg;
      }

      {      
         auto_ptr<SipMessage> msg1(TestSupport::makeMessage(buff.c_str()));
         MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
         assert(mpc);
         assert(mpc->parts().size() == 2);
         PlainContents *f21 = dynamic_cast<PlainContents*>(mpc->parts().front());
         assert(f21);
         f21->getBodyData();
         MultipartRelatedContents::Parts::const_iterator i = ++mpc->parts().begin();
         PlainContents *f22 = dynamic_cast<PlainContents*>(*i);
         assert(f22);
         f22->getBodyData();

         assert(f11->getBodyData() == f21->getBodyData());
         assert(f12->getBodyData() == f22->getBodyData());
      }
   }

   cerr << endl << "All OK" << endl;
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
