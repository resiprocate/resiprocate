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
      assert(uri.user() == "fluffy");
      assert(uri.userParameters() == "x-utag=foo");
      assert(uri.host() == "iii.ca");

      Data out(Data::from(uri));
      assert(out == "sip:fluffy;x-utag=foo@iii.ca");
   }

   {
      Uri uri("sip:fluffy;x-utag=foo:password@iii.ca");
      assert(uri.user() == "fluffy");
      assert(uri.userParameters() == "x-utag=foo");
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
      assert(uri.user() == "");
      assert(uri.userParameters() == "");
      assert(uri.host() == "");
      assert(uri.password() == "");

      Data out(Data::from(uri));
      cerr << "!! " << out << endl;
      assert(out == "sip:");
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
   cerr << endl << "All OK" << endl;
}
