#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory>
#include <iostream>

#include "TestSupport.hxx"
#include "resip/stack/UnknownParameterType.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;
   Log::initialize(Log::Cerr, l, argv[0]);
   initNetwork();
   
   {
      Uri uri("sip:speedy_AT_home.com@whistler.gloo.net:5062");
      uri.scheme() = Symbols::Pres;
      assert(Data::from(uri) == "pres:speedy_AT_home.com@whistler.gloo.net:5062");
   }
   {
       const char * a = "alice";
       const char * e = "example.com";

       NameAddr alice;
       alice.uri().user() = a;
       alice.uri().host() = e;

       NameAddr realm;
       realm.uri().host() = e;

       NameAddr aliceContact;
       aliceContact.uri().user() = a;
       aliceContact.uri().host() = "127.1.0.1";
       aliceContact.uri().port() = 32678;


       
       auto_ptr<SipMessage> msg(Helper::makeRegister(alice,alice,aliceContact));

       cout << *msg << endl;

       // Make the data
       NameAddr original(aliceContact);
       original.uri().param(UnknownParameterType("x")) = Data("\"1\"");

       Uri tmp = original.uri();
       Data buf;
       DataStream oldNA(buf);
       oldNA << Symbols::LA_QUOTE;
       oldNA << original.uri().scheme();
       oldNA << Symbols::COLON;
       oldNA << original.uri().getAor();
       oldNA << Symbols::RA_QUOTE;
       oldNA.flush();

       NameAddr modified;
       modified.uri() = tmp; // copy parameters;
       modified.uri().host() = e;
       modified.uri().port() = 65530;
       modified.uri().user() = "alphabet-soup";

       Data gruuData ( Data::from(modified.uri())) ;
       msg->header(h_Contacts).back().param(p_pubGruu) = gruuData;


       cout << *msg << endl;

       Uri s1("sip:alice@example.com;gr=\"foo@example.com\"");
       Uri s2("sip:alice@example.com;gr=\"foo@example.com\"");
       assert(s1.param(p_gr) == Data("foo@example.com"));
       assert(s2.param(p_gr) == Data("foo@example.com"));
       cout << s1 << endl;
       cout << s2 << endl;
       cout << endl;
       Uri s3("sip:alice@example.com");
       s3.param(UnknownParameterType("foo")) = Data("value");
       Uri s4("sip:alice@example.com");
       s4.param(UnknownParameterType("foo")) = Data("\"value\"");
       cout << "s3's param =" << s3.param(UnknownParameterType("foo")) << endl;
       cout << "s4's param =" << s4.param(UnknownParameterType("foo")) << endl;
       cout << "s3 = " << s3 << endl;
       cout << "s4 = " << s4 << endl;
       Uri s5(s4);
       cout << "s5 = " << s5 << endl;
       Data s5d(Data::from(s5));
       cout << "s5d = " << s5d << endl;

       Uri s6("sip:bob@example.com");
       s6.host();

       Uri s7("sip:testproxy.example.com");
	   assert (s7.user().empty());
	   s7.user() = "test";
	   assert (!s7.user().empty());
	   s7.user().clear();
	   assert (s7.user().empty());

       NameAddr na1;
       na1.uri().user() = "alice";
       na1.uri().host() = "example.com";

       Data q("\"");
       na1.param(UnknownParameterType("foo")) = Data(q + Data::from(s6) +q);
       NameAddr na2(na1);
       cout << "na1=" << na1 << endl;
       cout << "na2=" << na2 << endl;
       
   }
   //assert(0);
   {
      // Test order irrelevance of known parameters
      Uri sip1("sip:user@domain;ttl=15;method=foo");
      Uri sip2("sip:user@domain;method=foo;ttl=15");

      cerr << "!!" << sip1.host() << endl;
      cerr << "!!" << sip2.host() << endl;

      assert (sip1 == sip2);
      assert (sip2 == sip1);

#ifdef USE_NETNS
      cerr << "Testing NETNS" << endl;

      sip1.netNs() = "ns1";
      assert(sip1.netNs() == "ns1");
      assert(!(sip1 == sip2));
      assert(!(sip2 == sip1));
      Uri sip3(sip1);
      assert(sip1 == sip3);
      assert(!(sip2 == sip3));
      assert(!(sip3 == sip2));
      sip2 = sip1;
      assert(sip1 == sip2);
      assert(sip2 == sip1);
#endif
   }

   {
      assert(DnsUtil::isIpV6Address("::1"));
   }

#ifdef USE_IPV6
   {
      cerr << "!! " << DnsUtil::canonicalizeIpV6Address("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210") << endl;
      assert(DnsUtil::canonicalizeIpV6Address("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210") ==
             "fedc:ba98:7654:3210:fedc:ba98:7654:3210");
   }

   {
      cerr << "!! " << DnsUtil::canonicalizeIpV6Address("5f1b:df00:ce3e:e200:20:800:121.12.131.12") << endl;
      assert(DnsUtil::canonicalizeIpV6Address("5f1b:df00:ce3e:e200:20:800:121.12.131.12") ==
             "5f1b:df00:ce3e:e200:20:800:790c:830c");
   }

   {
      cerr << "!! " << DnsUtil::canonicalizeIpV6Address("5f1B::20:800:121.12.131.12") << endl;
      assert(DnsUtil::canonicalizeIpV6Address("5f1B::20:800:121.12.131.12") ==
             "5f1b::20:800:790c:830c");
   }
   
   {
      Uri uri("sip:[5f1b:df00:ce3e:e200:20:800:121.12.131.12]");

      cerr << "!! " << uri.host() << endl;
      assert(uri.host() == "5f1b:df00:ce3e:e200:20:800:121.12.131.12");
      cerr << "!! " << Data::from(uri) << endl;
      assert(Data::from(uri) == "sip:[5f1b:df00:ce3e:e200:20:800:121.12.131.12]");
   }

   {
      Uri uri("sip:user@[5f1b:df00:ce3e:e200:20:800:121.12.131.12]");

      cerr << "!! " << uri.host() << endl;
      assert(uri.host() == "5f1b:df00:ce3e:e200:20:800:121.12.131.12");
      cerr << "!! " << Data::from(uri) << endl;
      assert(Data::from(uri) == "sip:user@[5f1b:df00:ce3e:e200:20:800:121.12.131.12]");
   }
#endif
   
   {
      Uri uri("sips:192.168.2.12");

      assert(uri.scheme() == "sips");
      assert(uri.password() == "");
      assert(uri.userParameters() == "");
      assert(uri.host() == "192.168.2.12");
      assert(uri.port() == 0);
   }

   {
      Uri uri("sips:host.foo.com");
      assert(uri.scheme() == "sips");
      assert(uri.password() == "");
      assert(uri.userParameters() == "");
      assert(uri.host() == "host.foo.com");
      assert(uri.port() == 0);
   }

   {
      Uri uri("sip:user;x-v17:password@host.com:5555");

      cerr << "user!!" << uri.user() << endl;
      cerr << "password!!" << uri.password() << endl;
      cerr << "userParams!!" << uri.userParameters() << endl;

      assert(uri.scheme() == "sip");
      assert(uri.user() == "user;x-v17");
      assert(uri.password() == "password");
      assert(uri.userParameters() == "");
      assert(uri.host() == "host.com");
      assert(uri.port() == 5555);
   }

   {
      // test bad parses
      try
      {
         Uri("noscheme@foo.com:1202");
         assert(false);
      }
      catch (ParseException& e)
      {
      }
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
      Uri sip(Uri::fromTel(tel, Uri("sip:company.com")));

      cerr << "!! " << Data::from(sip) << endl;
      assert(Data::from(sip) == "sip:+358-555-1234567;isub=1411;postd=pp2@company.com;user=phone");
   }

   {
      Uri tel("tel:+358-555-1234567;foo=bar;aaaa=baz;pOstd=pP2;isUb=1411");
      Uri sip(Uri::fromTel(tel, Uri("sip:company.com")));

      cerr << "!! " << Data::from(sip) << endl;
      assert(Data::from(sip) == "sip:+358-555-1234567;isub=1411;postd=pp2;aaaa=baz;foo=bar@company.com;user=phone");
   }
   
   {
      Uri tel("tel:+358-555-1234567;postd=pp22");
      Uri sip(Uri::fromTel(tel, Uri("sip:foo.com")));
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
      Uri sip1(Uri::fromTel(tel1, Uri("sip:foo.com")));
      Uri sip2(Uri::fromTel(tel2, Uri("sip:foo.com")));
      assert (sip1 == sip2);
      assert (Data::from(sip1) == "sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
      assert (Data::from(sip2) == "sip:+358-555-1234567;postd=pp22@foo.com;user=phone");
   }
   {
      Uri tel1("tel:+358-555-1234567;tsp=a.b;phone-context=5");
      Uri tel2("tel:+358-555-1234567;phone-context=5;tsp=a.b");
      Uri sip1(Uri::fromTel(tel1, Uri("sip:foo.com")));
      Uri sip2(Uri::fromTel(tel2, Uri("sip:foo.com")));
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
      assert (sip2 == sip1);
   }

   {
      // Test order irrelevance of known parameters
      Uri sip1("sip:user@domain;ttl=15;method=foo");
      Uri sip2("sip:user@domain;method=foo;ttl=15");

      assert (sip1 == sip2);
      assert (sip2 == sip1);
   }
   

   // tests from 3261 19.1.4
   {
      Uri sip1("sip:alice@atlanta.com;transport=TCP");
      Uri sip2("sip:alice@AtLanTa.CoM;Transport=tcp");

      assert(sip1 == sip2);
      assert(sip2 == sip1);
   }

   {
      Uri sip1("sip:carol@chicago.com");
      Uri sip2("sip:carol@chicago.com;newparam=5");
      Uri sip3("sip:carol@chicago.com;security=on");

      assert(sip1 == sip2);
      assert(sip2 == sip1);
      assert(sip2 == sip3);
      assert(sip3 == sip2);
      assert(sip3 == sip1);
      assert(sip1 == sip3);
   }

   {
      Uri sip1("sip:biloxi.com;transport=tcp;method=REGISTER?to=sip:bob%40biloxi.com");
      Uri sip2("sip:biloxi.com;method=REGISTER;transport=tcp?to=sip:bob%40biloxi.com");

      assert(sip1 == sip2);
      assert(sip2 == sip1);
   }

  {
     Uri sip1("sip:alice@atlanta.com?subject=project%20x&priority=urgent");
     Uri sip2("sip:alice@atlanta.com?priority=urgent&subject=project%20x");

     assert(sip1 == sip2);
     assert(sip2 == sip1);
  }

  {
     Uri sip1("SIP:ALICE@AtLanTa.CoM;Transport=udp"); // (different usernames)
     Uri sip2("sip:alice@AtLanTa.CoM;Transport=UDP");

     assert(sip1 != sip2);
  }

  {
     Uri sip1("sip:bob@biloxi.com"); // (can resolve to different ports)
     Uri sip2("sip:bob@biloxi.com:5060");

     assert(sip1 != sip2);
  }     

  {
     Uri sip1("sip:bob@biloxi.com"); // (can resolve to different transports)
     Uri sip2("sip:bob@biloxi.com;transport=udp");

     assert(sip1 != sip2);
  }     

  {
     Uri sip1("sip:bob@biloxi.com"); // (can resolve to different port and transports)
     Uri sip2("sip:bob@biloxi.com:6000;transport=tcp");

     assert(sip1 != sip2);
  }     

  // !dlb! we ignore embedded headers at the moment
  if (false)
  {
     Uri sip1("sip:carol@chicago.com"); // (different header component)
     Uri sip2("sip:carol@chicago.com?Subject=next%20meeting");

     assert(sip1 != sip2);
  }     

  {
     Uri sip1("sip:bob@phone21.boxesbybob.com"); // (even though that's what phone21.boxesbybob.com resolves to)
     Uri sip2("sip:bob@192.0.2.4");  

     assert(sip1 != sip2);
  }

  {
    Uri sip1("sip:carol@chicago.com");
    Uri sip2("sip:carol@chicago.com;security=on");
    Uri sip3("sip:carol@chicago.com;security=off");

    assert(sip1 == sip2);
    assert(sip1 == sip3);
    assert(sip2 != sip3);
    assert(sip3 != sip2);
  }

  {
    Uri sip1("sip:carol@chicago.com");
    Uri sip2("sip:carol@chicago.com:5060");
    Uri sip3("sip:1.2.3.4");
    Uri sip4("sip:1.2.3.4:5070");
    Uri sip1a("sip:carol@chicago.com;user=phone;foo=bar");
    Uri sip2a("sip:carol@chicago.com:5060;user=phone;foo=bar");
    Uri sip3a("sip:1.2.3.4;user=phone;foo=bar");
    Uri sip4a("sip:1.2.3.4:5070;user=phone;foo=bar");

    DebugLog( << "sip1.getAor==" << sip1.getAor() );
    DebugLog( << "sip1.getAorNoPort==" << sip1.getAorNoPort() );
    DebugLog( << "sip2.getAor==" << sip2.getAor() );
    DebugLog( << "sip2.getAorNoPort==" << sip2.getAorNoPort() );
    
    assert( sip1.getAor() == Data("carol@chicago.com") );
    assert( sip2.getAor() == Data("carol@chicago.com:5060") );
    assert( sip3.getAor() == Data("1.2.3.4") );
    assert( sip4.getAor() == Data("1.2.3.4:5070") );

    assert( sip1a.getAor() == Data("carol@chicago.com") );
    assert( sip2a.getAor() == Data("carol@chicago.com:5060") );
    assert( sip3a.getAor() == Data("1.2.3.4") );
    assert( sip4a.getAor() == Data("1.2.3.4:5070") );

    assert( sip1.getAorNoPort() == Data("carol@chicago.com") );
    assert( sip2.getAorNoPort() == Data("carol@chicago.com") );
    assert( sip3.getAorNoPort() == Data("1.2.3.4") );
    assert( sip4.getAorNoPort() == Data("1.2.3.4") );

    assert( sip1a.getAorNoPort() == Data("carol@chicago.com") );
    assert( sip2a.getAorNoPort() == Data("carol@chicago.com") );
    assert( sip3a.getAorNoPort() == Data("1.2.3.4") );
    assert( sip4a.getAorNoPort() == Data("1.2.3.4") );
  }

  // Displayname parse tests
  {
    NameAddr sip1("\"DispName\" <sip:user@host.com>");
    NameAddr sip2("\"DispName \"<sip:user@host.com>");
    NameAddr sip3("\"  DispName\"<sip:user@host.com>");
    NameAddr sip4("DispName <sip:user@host.com>");
    NameAddr sip5("DispName<sip:user@host.com>");
    NameAddr sip6("  DispName <sip:user@host.com>");
    NameAddr sip7("  DispName<sip:user@host.com>");
    NameAddr sip8("  Disp Name  <sip:user@host.com>");

    DebugLog( << "sip1.displayName=='" << sip1.displayName() << "'" );
    DebugLog( << "sip2.displayName=='" << sip2.displayName() << "'" );
    DebugLog( << "sip3.displayName=='" << sip3.displayName() << "'" );
    DebugLog( << "sip4.displayName=='" << sip4.displayName() << "'" );
    DebugLog( << "sip5.displayName=='" << sip5.displayName() << "'" );
    DebugLog( << "sip6.displayName=='" << sip6.displayName() << "'" );
    DebugLog( << "sip7.displayName=='" << sip7.displayName() << "'" );
    DebugLog( << "sip8.displayName=='" << sip8.displayName() << "'" );
    
    assert( sip1.displayName() == Data("DispName") );
    assert( sip2.displayName() == Data("DispName ") );
    assert( sip3.displayName() == Data("  DispName") );
    assert( sip4.displayName() == Data("DispName") );  
    assert( sip5.displayName() == Data("DispName") );
    assert( sip6.displayName() == Data("DispName") );  
    assert( sip7.displayName() == Data("DispName") ); 
    assert( sip8.displayName() == Data("Disp Name") );   

    assert( sip1.uri().getAor() == Data("user@host.com") );
    assert( sip2.uri().getAor() == Data("user@host.com") );
    assert( sip3.uri().getAor() == Data("user@host.com") );
    assert( sip4.uri().getAor() == Data("user@host.com") );
    assert( sip5.uri().getAor() == Data("user@host.com") );
    assert( sip6.uri().getAor() == Data("user@host.com") );
    assert( sip7.uri().getAor() == Data("user@host.com") );
    assert( sip8.uri().getAor() == Data("user@host.com") );
  }

   // Embedded header testing
   {
       NameAddr addr("sip:user@domain.com?Call-Info=%3csip:192.168.0.1%3e%3banswer-after=0");
       //cout << addr << endl;
       assert(Data::from(addr.uri()) == "sip:user@domain.com?Call-Info=%3csip:192.168.0.1%3e%3banswer-after=0");
       //cout << "CallInfo:  " << addr.uri().embedded().header(h_CallInfos).front() << endl;
       assert(Data::from(addr.uri().embedded().header(h_CallInfos).front()) == "<sip:192.168.0.1>;answer-after=0");
   }

   // URI containing special character #
   // technically, # is meant to be encoded, but some systems (Asterisk,
   // some Cisco gear) seem to send this un-encoded
   // Some carriers insert # between a phone number and a billing prefix
   {
      Uri uri = Uri("sip:1234#00442031111111@lvdx.com");
      //cout << "Encoded correctly: " << uri << endl;
      assert(Data::from(uri) == "sip:1234%2300442031111111@lvdx.com");

      Uri::setUriUserEncoding('#', false);
      uri = Uri("sip:1234#00442031111111@lvdx.com");
      //cout << "Non Encoded # for compatibility: " << uri << endl;
      assert(Data::from(uri) == "sip:1234#00442031111111@lvdx.com");
      Uri::setUriUserEncoding('#', true);
   }

   {
      Uri uri = Uri("sip:1234#00442031111111;phone-context=+89@lvdx.com");
      assert(uri.userIsTelephoneSubscriber());
      Token telSub(uri.getUserAsTelephoneSubscriber());
      assert(telSub.value()=="1234#00442031111111");
      static ExtensionParameter p_phoneContext("phone-context");
      assert(telSub.exists(p_phoneContext));
      assert(telSub.param(p_phoneContext)=="+89");
      telSub.param(p_phoneContext)="+98";
      uri.setUserAsTelephoneSubscriber(telSub);
      assert(Data::from(uri) == "sip:1234%2300442031111111;phone-context=+98@lvdx.com");
   }

   {
      Uri uri = Uri("sip:+1-(234)-00442031111111@lvdx.com");
      assert(uri.userIsTelephoneSubscriber());
      Token telSub(uri.getUserAsTelephoneSubscriber());
      assert(telSub.value()=="+1-(234)-00442031111111");
      assert(!telSub.exists(p_extension));
      telSub.param(p_extension)="4545";
      uri.setUserAsTelephoneSubscriber(telSub);
      assert(Data::from(uri) == "sip:+1-(234)-00442031111111;ext=4545@lvdx.com");
   }

   {
      Uri uri = Uri("/");
      assert(uri.path() == "/");
      assert(Data::from(uri) == "/");
   }

   {
      Uri uri = Uri("/;p1=123;p2=456");
      assert(uri.path() == "/");
      assert(uri.param(UnknownParameterType("p1")) == Data("123"));
      assert(uri.param(UnknownParameterType("p2")) == Data("456"));
      assert(Data::from(uri) == "/;p1=123;p2=456");
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
