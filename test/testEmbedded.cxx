#include "resiprocate/os/DataStream.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Embedded.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace std;
using namespace resip;

int
main(int argc, char** argv)
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::INFO, argv[0]);

   {
      Data foo("abcdefghi1232454435");
      assert(Embedded::encode(foo) == foo);
   }

   {
      Data foo("abcdefghi1232454435^ * ");
      cerr << Embedded::encode(foo) << endl;
      assert(Embedded::encode(foo) == "abcdefghi1232454435^%20*%20");
   }

   {
      Data foo("abcdefghi1232454435^&*&");
      unsigned int c;
      char* res = Embedded::decode(foo, c);

      cerr << Data(res, c) << endl;
      assert(foo == Data(res, c));
      delete [] res;
   }

   {
      Data foo("!@#$%^&*() ?:=17qwerty");
      Data bar = Embedded::encode(foo);

      cerr << bar << endl;

      unsigned int c;
      char* res = Embedded::decode(bar, c);
      cerr << Data(res, c) << endl;
      assert(foo == Data(res, c));

      delete [] res;
   }

   {
      Data foo("!@#$%^&*() ?:=17qwerty");
      Data bar(Embedded::encode(Embedded::encode(foo)));

      cerr << bar << endl;

      unsigned int c;
      char* res1 = Embedded::decode(bar, c);
      Data rab(res1, c);
      char* res2 = Embedded::decode(rab, c);

      assert(foo == Data(res2, c));

      delete [] res1;
      delete [] res2;
   }

   {
      cerr << "Produce embedded, single header" << endl;
      Uri foo;
      foo.user() = "speedy";
      foo.host() = "cathaynetworks.com";
      foo.embedded().header(h_CSeq).method() = ACK;
      foo.embedded().header(h_CSeq).sequence() = 4178;

      Data buf;
      {
         DataStream str(buf);
         foo.encode(str);
      }
      cerr << endl << buf << endl;
      assert(buf == "sip:speedy@cathaynetworks.com?CSeq=4178%20ACK");
   }

   {
      cerr << "Produce embedded, multiple headers" << endl;
      Uri foo;
      foo.user() = "speedy";
      foo.host() = "cathaynetworks.com";
      foo.embedded().header(h_CSeq).method() = ACK;
      foo.embedded().header(h_CSeq).sequence() = 4178;

      Via via;
      BranchParameter branch = via.param(p_branch);

      branch.reset("fobbieBletch");
      via.transport() = "TLS";
      via.sentHost() = "cathay.com";
      via.sentPort() = 5066;
      via.param(p_branch) = branch;
      foo.embedded().header(h_Vias).push_back(via);

      branch.reset("bletchieFoo");
      via.transport() = "TCP";
      via.sentHost() = "ixolib.com";
      via.sentPort() = 5067;
      via.param(p_branch) = branch;
      foo.embedded().header(h_Vias).push_back(via);

      Data buf;
      {
         DataStream str(buf);
         foo.encode(str);
      }
      cerr << buf << endl;
      assert(buf == "sip:speedy@cathaynetworks.com?Via=SIP/2.0/TLS%20cathay.com:5066%3Bbranch%3Dz9hG4bK-c87542-fobbieBletch-1--c87542-&Via=SIP/2.0/TCP%20ixolib.com:5067%3Bbranch%3Dz9hG4bK-c87542-bletchieFoo-1--c87542-&CSeq=4178%20ACK");
   }

   {
      cerr << "Parse <Uri> with embedded" << endl;
      
      Data nad("bob<sips:bob@foo.com?CSeq=314159%20ACK>;tag=wd834f");
      NameAddr na(nad); 

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
   }

   {
      cerr << "Parse Uri with embedded" << endl;
      
      Data nad("sips:bob@foo.com;ttl=134?CSeq=314159%20ACK");
      NameAddr na(nad); 

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
      assert(na.uri().param(p_ttl) == 134);
   }

   {
      cerr << "Parse Uri with embedded followed by NameAddr parameter" << endl;
      
      Data nad("sips:bob@foo.com;ttl=134?CSeq=314159%20ACK;tag=17");
      NameAddr na(nad);

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
      assert(na.uri().param(p_ttl) == 134);
      assert(na.param(p_tag) == "17");
   }

   {
      cerr << "Parse Uri with multiple headers" << endl;
      
      Data nad("sip:speedy@cathaynetworks.com?Via=SIP/2.0/TLS%20cathay.com:5066%3Bbranch%3Dz9hG4bK-c87542-fobbieBletch--c87542-1&Via=SIP/2.0/TCP%20ixolib.com:5067%3Bbranch%3Dz9hG4bK-c87542-bletchieFoo--c87542-1&CSeq=4178%20ACK");
      NameAddr na(nad);

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_Vias));
      assert(na.uri().embedded().exists(h_CSeq));
      
      assert(na.uri().embedded().header(h_Vias).size() == 2);
      assert(na.uri().embedded().header(h_Vias).front().transport() == "TLS");
      assert((++(na.uri().embedded().header(h_Vias).begin()))->transport() == "TCP");
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 4178);
   }

   cerr << endl << "Tests OK" << endl;
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
