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
      // simple Token parse test
      char *org = "WuggaWuggaFoo";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      assert(tok.value() == org);
   }

   {
      // Token + parameters parse test
      char *org = "WuggaWuggaFoo;ttl=2";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      cerr << tok.value() << endl;
      assert(tok.value() == "WuggaWuggaFoo");
      cerr << tok.param(p_ttl) << endl;
      assert(tok.param(p_ttl) == 2);
      cerr << tok;
   }

   {
      // full on via parse
      char *viaString = /* Via: */ " SIP/2.0/UDP a.b.c.com:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv);
      cerr << via << endl;
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "a.b.c.com");
      assert(via.param(p_maddr) == "1.2.3.4");
   }

   {
      // URI parse
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
      // URI parse
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
      // URI parse
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
      // URI parse
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
      // URI parse
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
      // URI parse
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
      // Request Line parse
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
      // Request Line parse
      char *requestLineString = "INVITE sips:bob@foo.com;maddr=1.2.3.4 SIP/2.0";
      HeaderFieldValue hfv(requestLineString, strlen(requestLineString));

      RequestLine requestLine(&hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.uri().port() == 5060);
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.param(p_maddr) == "1.2.3.4");
      cerr << requestLine.getSipVersion() << endl;
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      // NameAddr parse
      char *nameAddrString = "sips:bob@foo.com";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
   {
      // NameAddr parse
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
      // NameAddr parse
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
      // NameAddr parse
      char *nameAddrString = "\"Bob   \\\" asd   \"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString, strlen(nameAddrString));

      NameAddr nameAddr(&hfv);
      assert(nameAddr.displayName() == "\"Bob   \\\" asd   \"");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
      assert(nameAddr.uri().port() == 5060);
   }
}


