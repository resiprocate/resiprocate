#include <assert.h>
#include <iostream>
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
      char *uriString = "sip:bob@foo.com";
      ParseBuffer pb(uriString);
      Uri uri;
      uri.parse(pb);
      assert(uri.scheme() == "sip");
      assert(uri.user() == "bob");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 5060);
   }

   {
      cerr << "URI parse, no displayName" << endl;
      char *uriString = "sips:foo.com";
      ParseBuffer pb(uriString);
      Uri uri;
      uri.parse(pb);
      assert(uri.scheme() == "sips");
      assert(uri.user() == "");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 5060);
   }

   {
      cerr << "URI parse, parameters" << endl;
      char *uriString = "sips:bob;param=gargle:password@foo.com";
      ParseBuffer pb(uriString);
      Uri uri;
      uri.parse(pb);
      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 5060);
   }

   {
      cerr << "URI parse, parameters, port" << endl;
      char *uriString = "sips:bob;param=gargle:password@foo.com:6000";
      ParseBuffer pb(uriString);
      Uri uri;
      uri.parse(pb);
      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 6000);
   }

   {
      cerr << "URI parse, parameters, correct termination check" << endl;
      char *uriString = "sips:bob;param=gargle:password@foo.com notHost";
      ParseBuffer pb(uriString);
      Uri uri;
      uri.parse(pb);
      assert(uri.scheme() == "sips");
      assert(uri.user() == "bob;param=gargle");
      assert(uri.password() == "password");
      cerr << "Uri:" << uri.host() << endl;
      assert(uri.host() == "foo.com");
   }

   {
      cerr << "Request Line parse" << endl;
      char *requestLineString = "INVITE sips:bob@foo.com SIP/2.0";
      HeaderFieldValue hfv(requestLineString, strlen(requestLineString));

      RequestLine requestLine(&hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.uri().port() == 5060);
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      cerr << "Request Line parse, parameters" << endl;
      char *requestLineString = "INVITE sips:bob@foo.com;maddr=1.2.3.4 SIP/2.0";
      HeaderFieldValue hfv(requestLineString, strlen(requestLineString));

      RequestLine requestLine(&hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.uri().port() == 5060);
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.uri().param(p_maddr) == "1.2.3.4");
      cerr << requestLine.getSipVersion() << endl;
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      cerr << "NameAddr parse" << endl;
      char *nameAddrString = "sips:bob@foo.com";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
   {
      cerr << "NameAddr parse, displayName" << endl;
      char *nameAddrString = "Bob<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
   {
      cerr << "NameAddr parse, quoted displayname" << endl;
      char *nameAddrString = "\"Bob\"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob\"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
   {
      cerr << "NameAddr parse, quoted displayname, embedded quotes" << endl;
      char *nameAddrString = "\"Bob   \\\" asd   \"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob   \\\" asd   \"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
   {
      cerr << "NameAddr parse, unquoted displayname, paramterMove" << endl;
      char *nameAddrString = "Bob<sips:bob@foo.com>;tag=456248;mobility=hobble";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
      
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
      cerr << "NameAddr parse, quoted displayname, parameterMove" << endl;
      char *nameAddrString = "\"Bob\"<sips:bob@foo.com>;tag=456248;mobility=hobble";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob\"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
      
      cerr << "Uri params: ";
      nameAddr.uri().encodeParameters(cerr) << endl;
      cerr << "Header params: ";
      nameAddr.encodeParameters(cerr) << endl;

      assert(nameAddr.uri().param(p_tag) == "456248");
      assert(nameAddr.uri().param(p_mobility) == "hobble");

      assert(nameAddr.exists(p_tag) == false);
      assert(nameAddr.exists(p_mobility) == false);
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
