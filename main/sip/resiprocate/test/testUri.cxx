#include <memory>
#include <iostream>

#include "TestSupport.hxx"
#include "resiprocate/UnknownParameterType.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::DEBUG;
   Log::initialize(Log::COUT, l, argv[0]);

   {
      // test q comparison
      Uri w1("sip:wombat@192.168.2.221:5062;transport=Udp;q=1.0");
      Uri w2("sip:wombat@192.168.2.221:5063;transport=Udp;q=0.5");
      Uri w3("sip:wombat@192.168.2.221:5063;transport=Udp;q=0.5");
      Uri w4("sip:wombat@192.168.2.221:5063;transport=Udp");

      Uri::GreaterQ gtQ;

      assert(gtQ(w1, w2));
      assert(!gtQ(w2, w1));
      assert(!gtQ(w2, w3));
      assert(!gtQ(w3, w2));
      assert(!gtQ(w1, w4));
      assert(!gtQ(w4, w1));
      assert(gtQ(w4, w3));
   }
   
   {
      Uri w1("sip:wombat@192.168.2.221:5062;transport=Udp");
      Uri w2("sip:wombat@192.168.2.221:5063;transport=Udp");
      assert(w1 != w2);
      assert(w1 < w2);
   }
   {
      Uri tel("tel:+358-555-1234567;pOstd=pP2;isUb=1411");
      assert(tel.user() == "+358-555-1234567");

      assert(Data::from(tel) == "tel:+358-555-1234567;pOstd=pP2;isUb=1411");
   }

   {
      Uri tel("tel:+358-555-1234567;pOstd=pP2;isUb=1411");
      Uri sip(Uri::fromTel(tel, "company.com"));

      cerr << "!! " << Data::from(sip) << endl;
      assert(Data::from(sip) == "sip:+358-555-1234567;isub=1411;postd=pp2@company.com;user=phone");
   }

   {
      Uri tel("tel:+358-555-1234567;foo=bar;aaaa=baz;pOstd=pP2;isUb=1411");
      Uri sip(Uri::fromTel(tel, "company.com"));

      cerr << "!! " << Data::from(sip) << endl;
      assert(Data::from(sip) == "sip:+358-555-1234567;isub=1411;postd=pp2;aaaa=baz;foo=bar@company.com;user=phone");
   }
   
   {
      Uri tel("tel:+358-555-1234567;postd=pp22");
      Uri sip(Uri::fromTel(tel, "foo.com"));
      assert(Data::from(sip) == "sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
   }
   {
      Uri tel1("tel:+358-555-1234567;postd=pp22");
      Uri tel2("tel:+358-555-1234567;POSTD=PP22");
      cerr << "tel1=" << tel1 << " user=" << tel1.user() << endl;
      cerr << "tel2=" << tel2 << " user=" << tel2.user() << endl;
      assert (tel1 == tel2);
   }
   {
      Uri tel1("sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
      Uri tel2("sip:+358-555-1234567;POSTD=PP22@foo.com;user=phone");
      assert (tel1 != tel2);
   }
   {
      Uri tel1("tel:+358-555-1234567;postd=pp22;isub=1411");
      Uri tel2("tel:+358-555-1234567;isub=1411;postd=pp22");
      // requires us to parse the user parameters
      //assert (tel1 == tel2);
   }
   {
      Uri tel1("sip:+358-555-1234567;postd=pp22;isub=1411@foo.com;user=phone");
      Uri tel2("sip:+358-555-1234567;isub=1411;postd=pp22@foo.com;user=phone");
      assert (tel1 != tel2);
   }
   {
      Uri tel1("tel:+358-555-1234567;postd=pp22");
      Uri tel2("tel:+358-555-1234567;POSTD=PP22");
      Uri sip1(Uri::fromTel(tel1, "foo.com"));
      Uri sip2(Uri::fromTel(tel2, "foo.com"));
      assert (sip1 == sip2);
      assert (Data::from(sip1) == "sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
      assert (Data::from(sip2) == "sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
   }
   {
      Uri tel1("tel:+358-555-1234567;tsp=a.b;phone-context=5");
      Uri tel2("tel:+358-555-1234567;phone-context=5;tsp=a.b");
      Uri sip1(Uri::fromTel(tel1, "foo.com"));
      Uri sip2(Uri::fromTel(tel2, "foo.com"));
      assert (sip1 == sip2);
      assert (Data::from(sip1) == "sip:+358-555-1234567;phone-context=5;tsp=a.b@foo.com;user=phone");
      assert (Data::from(sip2) == "sip:+358-555-1234567;phone-context=5;tsp=a.b@foo.com;user=phone");
   }

   {
      Uri uri("sip:fluffy@iii.ca:666");
      assert(uri.scheme() == "sip");
      assert(uri.user() == "fluffy");
      assert(uri.host() == "iii.ca");
      assert(uri.port() == 666);
   }
   
   {
      Uri uri("sip:fluffy@iii.ca;transport=tcp");
      assert(uri.param(p_transport) == "tcp");
   }
   
   {
      Uri uri("sips:fluffy@iii.ca;transport=tls");
      assert(uri.scheme() == "sips");
      assert(uri.param(p_transport) == "tls");
   }
   
   {
      Uri uri("sip:fluffy@iii.ca;transport=sctp");
      assert(uri.param(p_transport) == "sctp");
   }
   
   {
      Uri uri("sip:fluffy:password@iii.ca");
      assert(uri.password() == "password");
   }

   {
      Uri uri("sip:fluffy@iii.ca;user=phone;ttl=5;lr;maddr=1.2.3.4");
      assert(uri.param(p_ttl) == 5);
      assert(uri.exists(p_lr) == true);
      assert(uri.param(p_maddr) == "1.2.3.4");
      assert(uri.param(p_user) == "phone");
   }
 
   {
      Uri uri("sip:fluffy@iii.ca;x-fluffy=foo");
      assert(uri.exists(UnknownParameterType("x-fluffy")) == true);
      assert(uri.exists(UnknownParameterType("x-fufu")) == false);
      assert(uri.param(UnknownParameterType("x-fluffy")) == "foo");
   }
 
   {
      Uri uri("sip:fluffy@iii.ca;method=MESSAGE");
      assert(uri.param(p_method) == "MESSAGE");
   }

   {
      Uri uri("sip:+1(408) 444-1212:666@gw1");
      assert(uri.user() == "+1(408) 444-1212");
      assert(uri.password() == "666");
      assert(uri.host() == "gw1");
   }
 
   {
      Uri uri("sip:fluffy;x-utag=foo@iii.ca");
      assert(uri.user() == "fluffy;x-utag=foo");
      assert(uri.host() == "iii.ca");

      Data out(Data::from(uri));
      assert(out == "sip:fluffy;x-utag=foo@iii.ca");
   }

   {
      Uri uri("sip:fluffy;x-utag=foo:password@iii.ca");
      assert(uri.user() == "fluffy;x-utag=foo");
      assert(uri.host() == "iii.ca");
      assert(uri.password() == "password");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "sip:fluffy;x-utag=foo:password@iii.ca");
   }

   {
      Uri uri("tel:+14086661212");
      assert(uri.user() == "+14086661212");
      assert(uri.userParameters() == "");
      assert(uri.host() == "");
      assert(uri.password() == "");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "tel:+14086661212");
   }

   {
      Uri uri("tel:+14086661212;foo=bie");
      assert(uri.user() == "+14086661212");
      assert(uri.userParameters() == "foo=bie");
      assert(uri.host() == "");
      assert(uri.password() == "");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "tel:+14086661212;foo=bie");
   }

   {
      Uri uri("tel:+14086661212;");
      assert(uri.user() == "+14086661212");
      assert(uri.userParameters() == "");
      assert(uri.host() == "");
      assert(uri.password() == "");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "tel:+14086661212");
   }

   {
      Uri uri("sip:;:@");
      cerr << "uri.user() = " << uri.user() << endl;
      assert(uri.user() == ";");
      assert(uri.userParameters() == "");
      assert(uri.host() == "");
      assert(uri.password() == "");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "sip:;");
   }

   {
      Uri uri("tel:+1 (408) 555-1212");
      assert(uri.scheme() == "tel");
   }
   // Tests for user-less uris (was broken accidentally v1.44 Uri.cxx)
   {
     Data original("sip:1.2.3.4:5060");
     Data encoded;
     Uri uri(original);

     DataStream ds(encoded);
     uri.encode(ds);
     ds.flush();
     cout << "!! original data: " << original << endl;
     cout << "!! original uri : " << uri << endl;
     cout << "!! encoded  data: " << encoded << endl;

     assert( encoded == original );
   }
   {
      // Test order irrelevance of unknown parameters
      Uri sip1("sip:user@domain;foo=bar;baz=qux");
      Uri sip2("sip:user@domain;baz=qux;foo=bar");
      assert (sip1 == sip2);
   }
   cerr << endl << "All OK" << endl;
}
