#include <assert.h>
#include <iostream>
#include <sstream>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <sipstack/ParserCategories.hxx>
#include <string.h>
#include <util/ParseBuffer.hxx>
#include <sipstack/Uri.hxx>

using namespace std;
using namespace Vocal2;

int
main(int arc, char** argv)
{
   {
      // test header hash
      for (int i = Headers::CSeq; i < Headers::UNKNOWN; i++)
      {
         assert(Headers::getType(Headers::HeaderNames[i].c_str(), Headers::HeaderNames[i].size()) == i);
      }
   }

   {
      // test parameter hash
      for (int i = ParameterTypes::transport; i < ParameterTypes::UNKNOWN; i++)
      {
         assert(ParameterTypes::getType(ParameterTypes::ParameterNames[i].c_str(), ParameterTypes::ParameterNames[i].size()) == i);
      }
   }
   
   {
      cerr << "simple Token parse test" << endl;
      char *org = "WuggaWuggaFoo";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      assert(tok.value() == org);
   }

   {
      cerr << "Token + parameters parse test" << endl;
      char *org = "WuggaWuggaFoo;ttl=2";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      assert(tok.value() == "WuggaWuggaFoo");
      assert(tok.param(p_ttl) == 2);
   }

   {
      cerr << "full on via parse" << endl;
      char *viaString = /* Via: */ " SIP/2.0/UDP a.b.c.com:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv);
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "a.b.c.com");
      assert(via.param(p_maddr) == "1.2.3.4");
   }

   {
      cerr << "URI parse" << endl;
      Data uriString = "sip:bob@foo.com";
      ParseBuffer pb(uriString.data(), uriString.size());
      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();
      assert(uri.scheme() == "sip");
      assert(uri.user() == "bob");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 0);
   }

   {
      cerr << "URI parse, no displayName" << endl;
      Data uriString = "sips:foo.com";
      ParseBuffer pb(uriString.data(), uriString.size());
      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();
      assert(uri.scheme() == "sips");
      assert(uri.user() == "");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 0);
   }

   {
      cerr << "URI parse, parameters" << endl;
      Data uriString = "sips:bob;param=gargle:password@foo.com";
      ParseBuffer pb(uriString.data(), uriString.size());
      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();

      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      assert(uri.host() == "foo.com");
   }

   {
      cerr << "URI parse, parameters, port" << endl;
      Data uriString = "sips:bob;param=gargle:password@foo.com:6000";
      ParseBuffer pb(uriString.data(), uriString.size());

      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();

      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 6000);
   }

   {
      cerr << "URI parse, parameters, correct termination check" << endl;
      Data uriString = "sips:bob;param=gargle:password@foo.com notHost";
      ParseBuffer pb(uriString.data(), uriString.size());

      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();

      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      cerr << "Uri:" << uri.host() << endl;
      assert(uri.host() == "foo.com");
   }

   {
      cerr << "URI parse, transport parameter" << endl;
      Data uriString = "sip:bob@biloxi.com;transport=udp";
      ParseBuffer pb(uriString.data(), uriString.size());

      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();

      assert(uri.param(p_transport) == "udp");
   }
   
   cerr << "URI comparison tests" << endl;
   {
      Data uriStringA("sip:carol@chicago.com");
      Data uriStringB("sip:carol@chicago.com;newparam=5");
      Data uriStringC("sip:carol@chicago.com;security=on");
      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());
      ParseBuffer pc(uriStringC.data(), uriStringC.size());
      NameAddr nA, nB, nC;

      nA.parse(pa);
      nB.parse(pb);
      nC.parse(pc);
      
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();
      Uri& uC = nC.uri();
      
      assert(uA == uB);
      assert(uB == uC);
      assert(uA == uC);
   }
   {
      Data uriStringA = "sip:biloxi.com;transport=tcp;method=REGISTER";
      Data uriStringB = "sip:biloxi.com;method=REGISTER;transport=tcp";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());

      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();

      assert(uA == uB);
   }
   {
      Data uriStringA = "sip:alice@atlanta.com";
      Data uriStringB = "sip:alice@atlanta.com";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());

      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();

      assert(uA == uB);
   }
   {
      Data uriStringA = "sip:alice@AtLanTa.CoM;Transport=UDP";
      Data uriStringB = "SIP:ALICE@AtLanTa.CoM;Transport=udp";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());

      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();      

      assert(uA != uB);
   }
   {
      Data uriStringA = "sip:bob@192.0.2.4";
      Data uriStringB = "sip:bob@phone21.boxesbybob.com";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());
      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();
      assert(uA != uB);
   }
   {
      Data uriStringA = "sip:bob@biloxi.com:6000;transport=tcp";
      Data uriStringB = "sip:bob@biloxi.com";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());
      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();
      assert(uA != uB);
   }
   {
      Data uriStringA = "sip:bob@biloxi.com;transport=udp";
      Data uriStringB = "sip:bob@biloxi.com";

      ParseBuffer pa(uriStringA.data(), uriStringA.size());
      ParseBuffer pb(uriStringB.data(), uriStringB.size());
      NameAddr nA, nB;
      nA.parse(pa);
      nB.parse(pb);
      Uri& uA = nA.uri();
      Uri& uB = nB.uri();
      cerr << "A: " << uA << endl;
      cerr << "B: " << uB << endl;
      cerr << "A:exists(transport) " << uA.exists(p_transport) << endl;
      assert(uA != uB);
   }

   { //embedded header comparison, not supported yet
//      char *uriStringA = "sip:carol@chicago.com?Subject=next%20meeting";
//      char *uriStringB = "sip:carol@chicago.com";

//      ParseBuffer pa(uriStringA, strlen(uriStringA));
//      ParseBuffer pb(uriStringB, strlen(uriStringB));
//      Uri uA, uB;
//      uA.parse(uriStringA);
//      uB.parse(uriStringB);
//      assert(uA != uB);
   }

   {
      cerr << "Request Line parse" << endl;
      Data requestLineString("INVITE sips:bob@foo.com SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(&hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      cerr << "Request Line parse, parameters" << endl;
      Data requestLineString("INVITE sips:bob@foo.com;maddr=1.2.3.4 SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(&hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.uri().param(p_maddr) == "1.2.3.4");
      cerr << requestLine.getSipVersion() << endl;
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      cerr << "NameAddr parse" << endl;
      Data nameAddrString("sips:bob@foo.com");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      cerr << "NameAddr parse, displayName" << endl;
      Data nameAddrString("Bob<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      cerr << "NameAddr parse, quoted displayname" << endl;
      Data nameAddrString = "\"Bob\"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob\"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      cerr << "NameAddr parse, quoted displayname, embedded quotes" << endl;
      Data nameAddrString("\"Bob   \\\" asd   \"<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob   \\\" asd   \"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      cerr << "NameAddr parse, unquoted displayname, paramterMove" << endl;
      Data nameAddrString("Bob<sips:bob@foo.com>;tag=456248;mobility=hobble");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      cerr << "Uri params: ";
      nameAddr.uri().encodeParameters(cerr) << endl;
      cerr << "Header params: ";
      nameAddr.encodeParameters(cerr) << endl;
      assert(nameAddr.param(p_tag) == "456248");
      assert(nameAddr.param(p_mobility) == "hobble");

      assert(nameAddr.uri().exists(p_tag) == false);
      assert(nameAddr.uri().exists(p_mobility) == false);
   }
   {
      cerr << "NameAddr parse, quoted displayname, parameterMove" << endl;
      Data nameAddrString("\"Bob\"<sips:bob@foo.com>;tag=456248;mobility=hobble");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob\"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      cerr << "Uri params: ";
      nameAddr.uri().encodeParameters(cerr) << endl;
      cerr << "Header params: ";
      nameAddr.encodeParameters(cerr) << endl;

      assert(nameAddr.param(p_tag) == "456248");
      assert(nameAddr.param(p_mobility) == "hobble");

      assert(nameAddr.uri().exists(p_tag) == false);
      assert(nameAddr.uri().exists(p_mobility) == false);
   }
   {
      cerr << "NameAddr parse, unquoted displayname, paramterMove" << endl;
      Data nameAddrString("Bob<sips:bob@foo.com;tag=456248;mobility=hobble>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      cerr << "Uri params: ";
      nameAddr.uri().encodeParameters(cerr) << endl;
      cerr << "Header params: ";
      nameAddr.encodeParameters(cerr) << endl;
      assert(nameAddr.uri().param(p_tag) == "456248");
      assert(nameAddr.uri().param(p_mobility) == "hobble");

      assert(nameAddr.exists(p_tag) == false);
      assert(nameAddr.exists(p_mobility) == false);
   }
   {
      cerr << "NameAddr parse, unquoted displayname, paramterMove" << endl;
      Data nameAddrString("Bob<sips:bob@foo.com;mobility=\"hobb;le\";tag=\"true;false\">");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      cerr << "Uri params: ";
      nameAddr.uri().encodeParameters(cerr) << endl;
      cerr << "Header params: ";
      nameAddr.encodeParameters(cerr) << endl;
      assert(nameAddr.uri().param(p_mobility) == "hobb;le");
      assert(nameAddr.uri().param(p_tag) == "true;false");
      //      assert("true;false" == nameAddr.uri().param(Data("useless")));

      assert(nameAddr.exists(p_mobility) == false);
   }
   {
      cerr << "NameAddr parse" << endl;
      Data nameAddrString("sip:101@localhost:5080;transport=UDP");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "");
      assert(nameAddr.uri().scheme() == "sip");
      assert(nameAddr.uri().user() == "101");
   }
   {
      cerr << "NameAddr parse, no user in uri" << endl;
      Data nameAddrString("sip:localhost:5070");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "");
      assert(nameAddr.uri().scheme() == "sip");
      assert(nameAddr.uri().host() == "localhost");
      assert(nameAddr.uri().user() == "");
      assert(nameAddr.uri().port() == 5070);
   }
   {
      cerr << "StatusLine, no reason code" << endl;
      Data statusLineString("SIP/2.0 100 ");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(&hfv);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }
   {
      cerr << "StatusLine, no reason code" << endl;
      Data statusLineString("SIP/2.0 100");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(&hfv);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }

   {
     char* authorizationString = "Digest realm=\"66.100.107.120\", username=\"1234\", nonce=1011235448   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(&hfv);

      cerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "Digest");
      cerr << "   realm: " <<  auth.param("realm") << endl;
      assert(auth.param("realm") == "66.100.107.120"); 
      assert(auth.param("username") == "1234"); 
      assert(auth.param("nonce") == "1011235448"); 
      assert(auth.param("uri") == "sip:66.100.107.120"); 
      assert(auth.param("algorithm") == "MD5"); 
      assert(auth.param("response") == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      stringstream s;
      auth.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "Digest realm=\"66.100.107.120\",username=\"1234\",nonce=1011235448,uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }

   {
     char* authorizationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=1011235448   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(&hfv);

      //      cerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "");
      //      cerr << "   realm: " <<  auth.param("realm") << endl;
      assert(auth.param("realm") == "66.100.107.120"); 
      assert(auth.param("username") == "1234"); 
      assert(auth.param("nonce") == "1011235448"); 
      assert(auth.param("uri") == "sip:66.100.107.120"); 
      assert(auth.param("algorithm") == "MD5"); 
      assert(auth.param("response") == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      stringstream s;
      auth.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "realm=\"66.100.107.120\",username=\"1234\",nonce=1011235448,uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }

   {
     char* genericString = "<http://www.google.com>;purpose=icon;fake=true";
      HeaderFieldValue hfv(genericString, strlen(genericString));

      GenericURI generic(&hfv);

      assert(generic.uri() == "http://www.google.com");
      cerr << generic.param(p_purpose) << endl;
      assert(generic.param(p_purpose) == "icon");
      assert(generic.param("fake") == "true");

      stringstream s;
      generic.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "<http://www.google.com>;purpose=icon;fake=true");
   }


   cerr << "\nTEST OK" << endl;
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
