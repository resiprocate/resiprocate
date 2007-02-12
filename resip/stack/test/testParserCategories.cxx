#include <assert.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/UnknownParameterType.hxx"
#include "resip/stack/ApiCheckList.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Logger.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define RESIP_COOKIE "-d8754z-"

class TR
{
   private:
      ostream& os;
      Data label;

      TR(const TR&);

      void show(const char * s)
      {
         os << s << ' ' << label << endl;
      }

      void start()
      {
         show("-->");
      }
      
      void end()
      {
         show("<--");
      }
      
   public:
      TR(Data  s,ostream& o = cerr ):os(o),label(s) { start(); }
      TR(const char* l,ostream& o = cerr):os(o),label(l) { start(); }
      ~TR() { end();}
};

int
main(int arc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);



   {
      TR _tr("Test poorly formed NameAddr by construction");

      try
      {
         NameAddr test("<jason_AT_example.com@10.0.0.1;transport=TCP>");
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << e << endl;
      }
   }
   
   {
      TR _tr("Test copy transport param");

      NameAddr test("<sip:jason_AT_example.com@10.0.0.1:5060;transport=TCP>");
      cerr << test << endl;
      NameAddr copy = test;
      cerr << copy << endl;
      assert(test.uri().exists(p_transport));
      assert(copy.uri().exists(p_transport));
      
      assert(test.uri().param(p_transport) == copy.uri().param(p_transport));
   }
   
   {
      TR _tr("Test iterator erase in ParserContainer");

      NameAddrs nameAddrs;

      nameAddrs.push_back(NameAddr(Uri("sip:first@first.com")));
      nameAddrs.push_back(NameAddr(Uri("sip:second@second.com")));
      nameAddrs.push_back(NameAddr(Uri("sip:third@third.com")));
      nameAddrs.push_back(NameAddr(Uri("sip:fourth@fourth.com")));

      assert(nameAddrs.size() == 4);

      NameAddrs::iterator i = nameAddrs.begin();
      ++i;

      nameAddrs.erase(i);

      assert(nameAddrs.size() == 3);

      nameAddrs.erase(nameAddrs.begin());
      assert(nameAddrs.size() == 2);

      assert(nameAddrs.begin()->uri().user() == "third");
   }

   {
      TR _tr("Test find in ParserContainer");

      Tokens tokens;

      tokens.push_back(Token("Foo"));
      tokens.push_back(Token("Bar"));
      tokens.push_back(Token("Baz"));
      tokens.push_back(Token("Quux"));

      assert(tokens.find(Token("Foo")));
      assert(tokens.find(Token("Bar")));
      assert(tokens.find(Token("Baz")));
      assert(tokens.find(Token("Quux")));

      assert(!tokens.find(Token("Zab")));
   }

   {
      TR _tr("Test remove parameters that appear multiple times");
      Uri uri1("sip:a@b;xtype=1;maddr=local;xtype=2;maddr=remote;xtype=3;maddr=other");
      Uri uri2(uri1);

      uri1.remove(p_maddr);
      Data res1;
      {
         DataStream str(res1);
         str << uri1;
      }
      assert(res1 == "sip:a@b;xtype=1;xtype=2;xtype=3");

      UnknownParameterType p_xtype("xtype");
      uri2.remove(p_xtype);
      Data res2;
      {
         DataStream str(res2);
         str << uri2;
      }   
      assert(res2 == "sip:a@b;maddr=local;maddr=remote;maddr=other");
   }
   
   {
         {
            TR _tr("Test remove parameters that appear multiple times");
            Uri uri1("sips:bob@foo.com;transport=udp");
            Uri aor = uri1.getAorAsUri();

            Data res;
            {
               DataStream str(res);
               str << aor;
            }   
            cerr << res << endl;
            
            assert(res == "sips:bob@foo.com");
         }
         
   }


   {
      TR _tr("Test remove parameters that appear multiple times; mixed predefined and extensions");
      Uri uri1("sip:a@b;xtype=1;maddr=local;foo=bar;ttl=17;xtype=2;maddr=remote;foo=baz;ttl=42;xtype=3;maddr=other;foo=foo;ttl=111");
      Uri uri2(uri1);

      uri1.remove(p_maddr);
      Data res1;
      {
         DataStream str(res1);
         str << uri1;
      }
      assert(res1 == "sip:a@b;ttl=17;ttl=42;ttl=111;xtype=1;foo=bar;xtype=2;foo=baz;xtype=3;foo=foo");

      UnknownParameterType p_xtype("xtype");
      uri2.remove(p_xtype);
      Data res2;
      {
         DataStream str(res2);
         str << uri2;
      }   
      assert(res2 == "sip:a@b;maddr=local;ttl=17;maddr=remote;ttl=42;maddr=other;ttl=111;foo=bar;foo=baz;foo=foo");
   }

   {
      TR _tr("Test exists unknown parameter");

      Uri uri1("sip:a@b;xaudio");
      assert(uri1.exists(UnknownParameterType("xaudio")));
      assert(uri1.param(UnknownParameterType("xaudio")) == Data::Empty);
      
      Uri uri2("sip:a@b;a=b;xaudio");
      cerr << uri2.param(UnknownParameterType("a")) << endl;
      
      assert(uri2.exists(UnknownParameterType("xaudio")));
      assert(uri2.param(UnknownParameterType("xaudio")) == Data::Empty);

      Uri uri3("sip:a@b;xaudio;a=b");
      assert(uri3.exists(UnknownParameterType("xaudio")));
      assert(uri3.param(UnknownParameterType("xaudio")) == Data::Empty);
   }

   {
      TR _tr("Test non-quoted tokens displayname in NameAddr (torture test: 2.39)");
      Data data("A. Bell <sip:a.g.bell@bell-tel.com>;tag=459843");

      NameAddr legal(data);

      assert(legal.uri().host() == "bell-tel.com");

      cerr << "!!" << legal << endl;

      assert(legal.displayName() == "A. Bell");
   }

   {
      TR _tr("Test quoted displayname in NameAddr (torture test: 2.39)");
      Data data("\"A. Bell\" <sip:a.g.bell@bell-tel.com>;tag=459843");

      NameAddr legal(data);

      assert(legal.uri().host() == "bell-tel.com");

      cerr << "!!" << legal.displayName() << endl;

      assert(legal.displayName() == "A. Bell");
   }

   {
      TR _tr("Test NameAddr parameter handling");
      Data data("sip:foo@bar.com;user=phone");
      
      NameAddr original(data);
      assert(original.uri().exists(p_user));
      
      cerr << "!!" << original << endl;
   }

   {
      TR _tr("Test tel aor canonicalization");
      Data data("tel:+14156268178;pOstd=pP2;isUb=1411");
      
      Uri original(data);
      cerr << original.getAor() << endl;
      
      assert(original.getAor() == "+14156268178");
   }

   {
      TR _tr("Test aor canonicalization");
      Data data("sip:User@kElOwNa.GlOo.NeT:5666");
      Data data1("sip:User@KeLoWnA.gLoO.nEt:5666");
      
      Uri original(data);
      Uri original1(data1);

      cerr << "!! " << original.getAor() << " " << original1.getAor() << endl;
      assert(original.getAor() == original1.getAor());
   }

   {
      TR _tr("Test tel NameAddr");
      NameAddr n1("<tel:98267168>");
      cerr << n1.uri().user() << endl;
   }

   {
      TR _tr("Test empty NameAddr");
      NameAddr n1;
      NameAddr n2;
      assert (!(n1 < n2));
      assert (!(n2 < n1));
      assert (n1.uri().getAor() == n2.uri().getAor());
   }

   {
      TR _tr("Test NameAddr q value");

      NameAddr w("<sip:wombat@192.168.2.221:5062;transport=Udp>;expires=63;q=1");
#ifndef RESIP_FIXED_POINT
      assert(w.param(p_q) == 1.0);
#endif
      assert(w.param(p_q) == 1000);
      w.param(p_q) = 843;
      assert(w.param(p_q) <= 843);
#ifndef RESIP_FIXED_POINT
      assert(w.param(p_q) == 0.843);
      w.param(p_q) = 0.843;
      assert(w.param(p_q) == 843);
      assert(w.param(p_q) == 0.843);
      w.param(p_q) = 0.65;
      assert(w.param(p_q) == 650);
#endif
      w.param(p_q) = 0;
      assert(w.param(p_q) == 0);
   }
   
   {
      TR _tr("Test NameAddr comparison");

      NameAddr w1("<sip:wombat@192.168.2.221:5062;transport=Udp>;expires=63");
      NameAddr w2("<sip:wombat@192.168.2.221:5063;transport=Udp>;expires=66");
      assert(w1 < w2);
      assert (!(w2 < w1));
   }

   {
      TR _tr("Test parameter with spaces");
      Data txt("Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", reponse=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Auth auth(&hfv, Headers::Authorization);
      
      assert(auth.scheme() == "Digest");
      assert(auth.exists(p_realm));
   }

   {
      TR _tr("Test uri with no user");
      Data data("sip:kelowna.gloo.net");
      
      Uri original(data);
      cerr << original << endl;
      
      assert(Data::from(original) == data);
   }

   {
      TR _tr("Test assignment for NameAddr");
      NameAddr original(Data("\"Original\"<sip:orig@example.com>;tag=original"));
      (void)original.exists(p_tag);
      // force parse
      NameAddr newna(Data("\"new\"<sip:new@example.com>;tag=new"));
      (void)newna.exists(p_tag);
      cout << "original NameAddr: ->" << original << "<-"<< endl;
      cout << "new NameAddr     : ->" << newna << "<-" << endl;
      original = newna;
      cout << "original : ->" << original << "<-"<< endl;
      assert(Data::from(original) == Data::from(newna));
   }
   {
      TR _tr("Test typeless parameter copy");
      Token s = Token("jason");
      s.value() = "value";
      s.param(p_expires) = 17;
      s.param(p_lr);
      s.param(UnknownParameterType("foobie")) = "quux";

      Token s1;
      s1.value() = "other";
      s1.param(p_ttl) = 21;

      s.setParameter(s1.getParameterByEnum(ParameterTypes::ttl));
      assert(s.value() == "value");
      assert(s.param(p_ttl) == 21);
      assert(s.param(p_lr));
      assert(s.param(UnknownParameterType("foobie")) == "quux");
   }

   {
      TR a("Test typeless parameter overwrite" );
      Token s;
      s.value() = "value";
      s.param(p_expires) = 17;
      s.param(p_ttl) = 12;
      s.param(p_lr);
      s.param(UnknownParameterType("foobie")) = "quux";

      Token s1;
      s1.value() = "other";
      s1.param(p_ttl) = 21;

      s.setParameter(s1.getParameterByEnum(ParameterTypes::ttl));
      assert(s.value() == "value");
      assert(s.param(p_ttl) == 21);
      assert(s.param(p_lr));
      assert(s.param(UnknownParameterType("foobie")) == "quux");

      s.encode(cerr);
      cerr << endl;
   }

   {
      TR _tr( "Test StringCategory");
      Data stringString("Lame Agent");
      HeaderFieldValue hfv(stringString.data(), stringString.size());
      
      StringCategory str(&hfv, Headers::UNKNOWN);
      assert(str.value() == stringString);

      Data buff;
      {
         DataStream s(buff);
         str.encode(s);
      }
      cerr << buff << endl;
      assert(buff == stringString);

      StringCategory copy(str);
      assert(copy.value() == stringString);

      str.value() = "foo";
      assert(str.value() == "foo");
   }

   {
      TR _tr( "Test Token parameters");
      Token state;
      state.value() = Data("active");
      state.param(p_expires) = 666;
      cerr << state << endl;
   }

   {
      TR _tr( "StatusLine, with reason code");
      Data statusLineString("SIP/2.0 180 Ringing");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(&hfv, Headers::UNKNOWN);
      assert(statusLine.responseCode() == 180);
      cerr << statusLine.reason() << endl;
      assert(statusLine.reason() == "Ringing");
      assert(statusLine.getSipVersion() == "SIP/2.0");

      StatusLine copy(statusLine);
      assert(copy.responseCode() == 180);
      assert(copy.reason() == "Ringing");
      assert(copy.getSipVersion() == "SIP/2.0");
   }
   {
      Uri foo;
      // valgrind complains, but the problem moves when closely observed
      assert(foo.getAor().empty());
   }

#define checkHeaderName(_name) cerr << Headers::_name << " " << Headers::getHeaderName(Headers::_name) << " = " << #_name << endl /*;assert(isEqualNoCase(Headers::getHeaderName(Headers::_name), #_name))*/
   {
      // test header hash
      for (int i = Headers::CSeq; i < Headers::MAX_HEADERS; i++)
      {
         if (i == Headers::RESIP_DO_NOT_USE)
         {
            continue;
         }
         
         Data hdr = Headers::getHeaderName(i);
         if (!hdr.size()) continue;
         Data msg("Checking hash of: ");
         msg += hdr;
         TR _tr(msg);
         assert(Headers::getType(Headers::getHeaderName(i).c_str(), Headers::getHeaderName(i).size()) == i);
      }
      checkHeaderName(To);
      checkHeaderName(From);
      checkHeaderName(Via);
      checkHeaderName(CallID);
      checkHeaderName(CSeq);
      checkHeaderName(Route);
      checkHeaderName(RecordRoute);
      checkHeaderName(Contact);
      checkHeaderName(Subject);
      checkHeaderName(Expires);
      checkHeaderName(MaxForwards);
      checkHeaderName(Accept);
      checkHeaderName(AcceptEncoding);
      checkHeaderName(AcceptLanguage);
      checkHeaderName(AlertInfo);
      checkHeaderName(Allow);
      checkHeaderName(AuthenticationInfo);
      checkHeaderName(CallInfo);
      checkHeaderName(ContentDisposition);
      checkHeaderName(ContentEncoding);
      checkHeaderName(ContentLanguage);
      checkHeaderName(ContentTransferEncoding);
      checkHeaderName(ContentType);
      checkHeaderName(Date);
      checkHeaderName(InReplyTo);
      checkHeaderName(MinExpires);
      checkHeaderName(MIMEVersion);
      checkHeaderName(Organization);
      checkHeaderName(Priority);
      checkHeaderName(ProxyAuthenticate);
      checkHeaderName(ProxyAuthorization);
      checkHeaderName(ProxyRequire);
      checkHeaderName(ReplyTo);
      checkHeaderName(Require);
      checkHeaderName(RetryAfter);
      checkHeaderName(Server);
      checkHeaderName(SIPETag);
      checkHeaderName(SIPIfMatch);
      checkHeaderName(Supported);
      checkHeaderName(Timestamp);
      checkHeaderName(Unsupported);
      checkHeaderName(UserAgent);
      checkHeaderName(Warning);
      checkHeaderName(WWWAuthenticate);
      checkHeaderName(SubscriptionState);
      checkHeaderName(ReferTo);
      checkHeaderName(ReferredBy);
      checkHeaderName(Authorization);
      checkHeaderName(Replaces);
      checkHeaderName(Event);
      checkHeaderName(AllowEvents);
      checkHeaderName(SecurityClient);
      checkHeaderName(SecurityServer);
      checkHeaderName(SecurityVerify);
      checkHeaderName(ContentLength);
      checkHeaderName(ContentId);

   }

#define checkParameterName(_name) cerr << ParameterTypes::_name << " " << ParameterTypes::ParameterNames[ParameterTypes::_name] << " = " << #_name << endl/*;assert(isEqualNoCase(ParameterTypes::ParameterNames[ParameterTypes::_name], #_name))*/
   {
      checkParameterName(data);
      checkParameterName(control);
      checkParameterName(mobility);
      checkParameterName(description);
      checkParameterName(events);
      checkParameterName(priority);
      checkParameterName(methods);
      checkParameterName(schemes);
      checkParameterName(application);
      checkParameterName(video);
      checkParameterName(language);
      checkParameterName(type);
      checkParameterName(isFocus);
      checkParameterName(actor);
      checkParameterName(text);
      checkParameterName(extensions);
      checkParameterName(Instance);
      checkParameterName(gruu);

      checkParameterName(transport);
      checkParameterName(user);
      checkParameterName(method);
      checkParameterName(ttl);
      checkParameterName(maddr);
      checkParameterName(lr);
      checkParameterName(q);
      checkParameterName(purpose);
      checkParameterName(expires);
      checkParameterName(handling);
      checkParameterName(tag);
      checkParameterName(toTag);
      checkParameterName(fromTag);
      checkParameterName(duration);
      checkParameterName(branch);
      checkParameterName(received);
      checkParameterName(comp);
      checkParameterName(rport);
      checkParameterName(algorithm);
      checkParameterName(cnonce);
      checkParameterName(domain);
      checkParameterName(id);
      checkParameterName(nonce);
      checkParameterName(nc);
      checkParameterName(opaque);
      checkParameterName(realm);
      checkParameterName(response);
      checkParameterName(stale);
      checkParameterName(username);
      checkParameterName(qop);
      checkParameterName(uri);
      checkParameterName(retryAfter);
      checkParameterName(reason);
      checkParameterName(dAlg);
      checkParameterName(dQop);
      checkParameterName(dVer);
      checkParameterName(smimeType);
      checkParameterName(name);
      checkParameterName(filename);
      checkParameterName(protocol);
      checkParameterName(micalg);
      checkParameterName(boundary);
      checkParameterName(expiration);
      checkParameterName(size);
      checkParameterName(permission);
      checkParameterName(site);
      checkParameterName(directory);
      checkParameterName(mode);
      checkParameterName(server);
      checkParameterName(charset);
      checkParameterName(accessType);
      checkParameterName(profileType);
      checkParameterName(vendor);
      checkParameterName(model);
      checkParameterName(version);
      checkParameterName(effectiveBy);
      checkParameterName(document);
      checkParameterName(appId);
      checkParameterName(networkUser);
      
      checkParameterName(url);
      


      // test parameter hash
      for (int i = 0; i < ParameterTypes::MAX_PARAMETER; i++)
      {
         if (i != ParameterTypes::qopOptions &&
             i != ParameterTypes::qop )
         {
	    
            TR _tr( Data("Checking hash of: ") +  Data(ParameterTypes::ParameterNames[i]));
            assert(ParameterTypes::getType(ParameterTypes::ParameterNames[i].c_str(), 
                                           ParameterTypes::ParameterNames[i].size()) == i);
         }
      }

      assert(ParameterTypes::ParameterNames[ParameterTypes::qop] == "qop");
      assert(ParameterTypes::ParameterNames[ParameterTypes::qopOptions] == "qop");
   }
   
   {
      TR _tr( "simple Token parse test");
      char *org = "WuggaWuggaFoo";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv, Headers::UNKNOWN);
      assert(tok.value() == org);
   }

   {
      TR _tr( "Token + parameters parse test");
      char *org = "WuggaWuggaFoo;ttl=2";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv, Headers::UNKNOWN);
      assert(tok.value() == "WuggaWuggaFoo");
      assert(tok.param(p_ttl) == 2);
   }

   {
      TR _tr( "Test NameAddr(Data) constructor");
      
      Data nad("bob<sips:bob@foo.com>;tag=wd834f");
      NameAddr na(nad); 
      assert(na.uri().user() == "bob");
   }

   {
      TR _tr( "full on via parse");
      char *viaString = /* Via: */ " SIP/2.0/UDP a.b.c.com:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "a.b.c.com");
      assert(via.param(p_maddr) == "1.2.3.4");
   }

   {
      TR _tr("Test poorly formed DataParameter by construction");

      char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;tag=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(&hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << "Caught parse exception for Via" << endl;
      }
   }

   {
      TR _tr("Test poorly formed UnknownParameter by construction");

      char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;foobar=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(&hfv, Headers::UNKNOWN);
         via.sentPort();
         UnknownParameterType p_foobar("foobar");
         via.exists(p_foobar);
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << "Caught parse exception for Via" << endl;
      }
   }

   {
      TR _tr("Test poorly formed UInt32Parameter by construction");

      char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;duration=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(&hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << "Caught parse exception for Via " << endl;
      }
   }

   {
      TR _tr("Test poorly formed QuotedDataParameter by construction");

      char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;domain=\"";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(&hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << "Caught parse exception for Via " << endl;
      }
   }

#ifdef USE_IPV6
   {
      TR _tr( "full on via parse, IPV6");
      // !dlb! deal with maddr=[5f1b:df00:ce3e:e200:20:800:2b37:6426]
      char *viaString = /* Via: */ " SIP/2.0/UDP [5f1b:df00:ce3e:e200:20:800:2b37:6426]:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "5f1b:df00:ce3e:e200:20:800:2b37:6426");
      assert(via.param(p_maddr) == "1.2.3.4");
   }
#endif
   {
      TR _tr( "URI parse");
      Data uriString = "sip:bob@foo.com";
      ParseBuffer pb(uriString.data(), uriString.size());
      NameAddr to;
      to.parse(pb);
      Uri& uri = to.uri();
      cerr << "!! " << to << endl;
      assert(uri.scheme() == "sip");
      assert(uri.user() == "bob");
      assert(uri.host() == "foo.com");
      assert(uri.port() == 0);
   }

   {
      TR _tr( "URI parse, no displayName");
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
      TR _tr( "URI parse, parameters");
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
      TR _tr( "URI parse, parameters, port");
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
      TR _tr( "URI parse, parameters, correct termination check");
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
      TR _tr( "URI parse, transport parameter");
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
      TR _tr( "Request Line parse");
      Data requestLineString("INVITE sips:bob@foo.com SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(&hfv, Headers::UNKNOWN);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      cerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "Request Line parse tel");
      Data requestLineString("INVITE tel:4153331212 SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(&hfv, Headers::UNKNOWN);
      assert(requestLine.uri().scheme() == "tel");
      assert(requestLine.uri().user() == "4153331212");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "Request Line parse, parameters");
      Data requestLineString("INVITE sips:bob@foo.com;maddr=1.2.3.4 SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(&hfv, Headers::UNKNOWN);
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
      TR _tr( "NameAddr parse");
      Data nameAddrString("sips:bob@foo.com");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, displayName");
      Data nameAddrString("Bob<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, quoted displayname");
      Data nameAddrString = "\"Bob\"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, quoted displayname, embedded quotes");
      Data nameAddrString("\"Bob   \\\" asd   \"<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob   \\\" asd   ");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com>;tag=456248;mobility=\"hobble\"");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
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
      TR _tr( "NameAddr parse, quoted displayname, parameterMove");
      Data nameAddrString("\"Bob\"<sips:bob@foo.com>;tag=456248;mobility=\"hobble\"");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
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
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com;tag=456248;mobility=\"hobble\">");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
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
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com;mobility=\"hobb;le\";tag=\"true;false\">");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
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
      TR _tr( "NameAddr parse");
      Data nameAddrString("sip:101@localhost:5080;transport=UDP");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "");
      assert(nameAddr.uri().scheme() == "sip");
      assert(nameAddr.uri().user() == "101");
   }
   {
      TR _tr( "NameAddr parse, no user in uri");
      Data nameAddrString("sip:localhost:5070");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(&hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "");
      assert(nameAddr.uri().scheme() == "sip");
      assert(nameAddr.uri().host() == "localhost");
      assert(nameAddr.uri().user() == "");
      assert(nameAddr.uri().port() == 5070);
   }
   {
      TR _tr( "StatusLine, no reason code");
      Data statusLineString("SIP/2.0 100 ");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(&hfv, Headers::UNKNOWN);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "StatusLine, no reason code");
      Data statusLineString("SIP/2.0 100");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(&hfv, Headers::UNKNOWN);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr("Auth Schemes");
      char* authorizationString = "Digest realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(&hfv, Headers::UNKNOWN);

      cerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "Digest");
      cerr << "   realm: " <<  auth.param(p_realm) << endl;
      assert(auth.param(p_realm) == "66.100.107.120"); 
      assert(auth.param(p_username) == "1234"); 
      assert(auth.param(p_nonce) == "1011235448"); 
      assert(auth.param(p_uri) == "sip:66.100.107.120"); 
      assert(auth.param(p_algorithm) == "MD5"); 
      assert(auth.param(p_response) == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      stringstream s;
      auth.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "Digest realm=\"66.100.107.120\",username=\"1234\",nonce=\"1011235448\",uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }

   {
      TR _tr("More Auth");
      char* authorizationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(&hfv, Headers::UNKNOWN);

      cerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "");
      cerr << "   realm: " <<  auth.param(p_realm) << endl;
      assert(auth.param(p_realm) == "66.100.107.120"); 
      assert(auth.param(p_username) == "1234"); 
      assert(auth.param(p_nonce) == "1011235448"); 
      assert(auth.param(p_uri) == "sip:66.100.107.120"); 
      assert(auth.param(p_algorithm) == "MD5"); 
      assert(auth.param(p_response) == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      stringstream s;
      auth.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "realm=\"66.100.107.120\",username=\"1234\",nonce=\"1011235448\",uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }
   
   {
      TR _tr("Testing qop stuff");
      char* authenticationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, qop=\"auth,auth-int\"";
      char* authorizationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\", qop=auth";
      HeaderFieldValue authenHfv(authenticationString, strlen(authenticationString));
      HeaderFieldValue authorHfv(authorizationString, strlen(authorizationString));
      
      Auth wwwAuthen(&authenHfv, Headers::WWWAuthenticate);
      Auth pAuthen(&authenHfv, Headers::ProxyAuthenticate);
      Auth authInfo(&authorHfv, Headers::AuthenticationInfo);
      Auth pAuthor(&authorHfv, Headers::ProxyAuthorization);
      Auth author(&authorHfv, Headers::Authorization);

      assert(wwwAuthen.exists(p_qopOptions));
      assert(!wwwAuthen.exists(p_qop));
      assert(wwwAuthen.param(p_qopOptions)=="auth,auth-int");
      
      assert(pAuthen.exists(p_qopOptions));
      assert(!pAuthen.exists(p_qop));
      assert(pAuthen.param(p_qopOptions)=="auth,auth-int");
      
      assert(!authInfo.exists(p_qopOptions));
      assert(authInfo.exists(p_qop));
      assert(authInfo.param(p_qop)=="auth");
      
      assert(!pAuthor.exists(p_qopOptions));
      assert(pAuthor.exists(p_qop));
      assert(pAuthor.param(p_qop)=="auth");
      
      assert(!author.exists(p_qopOptions));
      assert(author.exists(p_qop));
      assert(author.param(p_qop)=="auth");
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            wwwAuthen.encode(str);
         }
         assert(encoded.find("qop=auth")==Data::npos);
         assert(encoded.find("qop=\"auth,auth-int\"")!=Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
         assert(encoded.find("qop=auth,auth-int")==Data::npos);
      }
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            pAuthen.encode(str);
         }
         assert(encoded.find("qop=auth")==Data::npos);
         assert(encoded.find("qop=\"auth,auth-int\"")!=Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
         assert(encoded.find("qop=auth,auth-int")==Data::npos);
      }
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            authInfo.encode(str);
         }
         assert(encoded.find("qop=auth")!=Data::npos);
         assert(encoded.find("qop=\"auth,auth-int\"")==Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
         assert(encoded.find("qop=auth,auth-int")==Data::npos);
      }
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            pAuthor.encode(str);
         }
         assert(encoded.find("qop=auth")!=Data::npos);
         assert(encoded.find("qop=\"auth,auth-int\"")==Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
         assert(encoded.find("qop=auth,auth-int")==Data::npos);
      }
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            author.encode(str);
         }
         assert(encoded.find("qop=auth")!=Data::npos);
         assert(encoded.find("qop=\"auth,auth-int\"")==Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
         assert(encoded.find("qop=auth,auth-int")==Data::npos);
      }
      
      Auth emptyAuthor;
      Auth emptyAuthen;
      
      emptyAuthor.param(p_qop)="auth";
      emptyAuthen.param(p_qopOptions)="auth";
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            emptyAuthor.encode(str);
         }
         assert(encoded.find("qop=auth")!=Data::npos);
         assert(encoded.find("qop=\"auth\"")==Data::npos);
      }
      
      {
         Data encoded;
         {
            oDataStream str(encoded);
            emptyAuthen.encode(str);
         }
         assert(encoded.find("qop=auth")==Data::npos);
         assert(encoded.find("qop=\"auth\"")!=Data::npos);
      }
   }

   {
      TR _tr("More Auth Encoding");

      Auth auth;
      Auth auth2;
      auth.scheme() = "Digest";
      auth.param(p_username) = "bob";

      auth2 = auth;
      Auth auth3(auth2);
      
      Data a;
      Data a1;
      Data a2;
      {
         DataStream s(a);
         s << auth;
      }
      {
         DataStream s(a1);
         s << auth;
      }
      {
         DataStream s(a2);
         s << auth;
      }
      
      assert(a == a1);
      assert(a1 == a2);
   }

   {
      TR _tr("Generic URI stuff");

      char* genericString = "<http://www.google.com>;purpose=icon;fake=true";
      HeaderFieldValue hfv(genericString, strlen(genericString));

      GenericUri generic(&hfv, Headers::UNKNOWN);

      assert(generic.uri() == "http://www.google.com");
      cerr << generic.param(p_purpose) << endl;
      assert(generic.param(p_purpose) == "icon");
      assert(generic.param(UnknownParameterType("fake")) == "true");

      stringstream s;
      generic.encode(s);

      cerr << s.str() << endl;
      
      assert(s.str() == "<http://www.google.com>;purpose=icon;fake=true");
   }

   {
      TR _tr("Date testing 1");
      char *dateString = "Mon, 04 Nov 2002 17:34:15 GMT";
      HeaderFieldValue hfv(dateString, strlen(dateString));
      
      DateCategory date(&hfv, Headers::UNKNOWN);

      assert(date.dayOfWeek() == Mon);
      assert(date.dayOfMonth() == 04);
      assert(date.month() == Nov);
      assert(date.year() == 2002);
      assert(date.hour() == 17);
      assert(date.minute() == 34);
      assert(date.second() == 15);

      stringstream s;
      date.encode(s);

      cerr << s.str() << endl;

      assert(s.str() == dateString);
      
      // copy ctor  not working in v1.94 ParserCategories.cxx
      
      stringstream s2;
      DateCategory otherDate(date);
      otherDate.encode(s2);
      cerr << "!! original date     : " << date << endl;
      cerr << "!! original string   : " << dateString << endl;
      cerr << "!! otherDate         : " << otherDate << endl;
      cerr << "!! encoded otherDate : " <<  s2.str() << endl;
      assert (s2.str() == dateString);

   }

   {
      TR _tr("Date testing 2");
      char *dateString = "  Sun  , 14    Jan 2222 07:04:05   GMT    ";
      HeaderFieldValue hfv(dateString, strlen(dateString));
      
      DateCategory date(&hfv, Headers::UNKNOWN);

      assert(date.dayOfWeek() == Sun);
      assert(date.dayOfMonth() == 14);
      assert(date.month() == Jan);
      assert(date.year() == 2222);
      assert(date.hour() == 07);
      assert(date.minute() == 04);
      assert(date.second() == 05);

      stringstream s;
      date.encode(s);
      assert(s.str() == "Sun, 14 Jan 2222 07:04:05 GMT");
   }


   {
      TR _tr("Mime types 1");

      char* mimeString = "application/sdp";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(&hfv, Headers::UNKNOWN);

      assert(mime.type() == "application");
      assert(mime.subType() == "sdp");

      stringstream s;
      mime.encode(s);
      assert(s.str() == mimeString);
   }


   {
      TR _tr("Mime types 2");
      char* mimeString = "text/html ; charset=ISO-8859-4";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(&hfv, Headers::UNKNOWN);

      assert(mime.type() == "text");
      assert(mime.subType() == "html");
      assert(mime.param(p_charset) == "ISO-8859-4");

      stringstream s;
      mime.encode(s);
      assert(s.str() == "text/html;charset=ISO-8859-4");
   }

   {
      TR _tr("Mime types 3");

      char* mimeString = "    text   /     html        ;  charset=ISO-8859-4";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(&hfv, Headers::UNKNOWN);

      assert(mime.type() == "text");
      assert(mime.subType() == "html");
      assert(mime.param(p_charset) == "ISO-8859-4");

      stringstream s;
      mime.encode(s);
      assert(s.str() == "text/html;charset=ISO-8859-4");
   }

   {
      TR _tr("Via 1");

      Via via;
      via.encode(cerr);
      cerr << endl;

      assert (via.param(p_branch).hasMagicCookie());
   }

   {
      TR _tr("Via 2");
      
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());

      stringstream s0;
      via.encode(s0);
      cerr << s0.str() << endl;
      assert(s0.str() == "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj");
      
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      
      stringstream s1;
      via.encode(s1);
      assert(s1.str() == "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj");
      
      via.param(p_branch).reset("jason");
      stringstream s2;
      via.encode(s2);
      cerr << "!! " << s2.str() << endl;
      assert(s2.str() == "SIP/2.0/UDP ;branch=z9hG4bK" RESIP_COOKIE "jason-1--" RESIP_COOKIE "");
      assert(via.param(p_branch).getTransactionId() == "jason");
   }

   {
      TR _tr("Via 3");
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
   }

   {
      TR _tr("Via 4");
      char* viaString = "SIP/2.0/UDP ;branch=oldassbranch";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (!via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "oldassbranch");
      
      stringstream s;
      via.encode(s);
      assert(s.str() == "SIP/2.0/UDP ;branch=oldassbranch");
      
      via.param(p_branch).reset("jason");
      stringstream s2;
      via.encode(s2);
      assert(s2.str() == "SIP/2.0/UDP ;branch=z9hG4bK" RESIP_COOKIE "jason-1--" RESIP_COOKIE "");
      assert(via.param(p_branch).getTransactionId() == "jason");
   }

   {
      TR _tr("Via 5 assignment with unknown parameter");
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;stid=abcd.2";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);

      Via via1;
      via1 = via;

      cerr << "!! "; via1.encode(cerr); cerr << endl;
      assert(via1.param(UnknownParameterType("stid")) == "abcd.2");
   }

   {
      TR _tr("Via 6 parse with known parameter");
      char* viaString = "SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "ec1e.0-1--" RESIP_COOKIE ";ttl=4\r\n";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "ec1e.0");
      cerr << "!! "; via.encode(cerr); cerr << endl;
      assert(via.param(p_ttl) == 4);
   }

   {
      TR _tr("Via 7 parse with unknown parameter");
      char* viaString = "SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "ec1e.0-1--" RESIP_COOKIE ";stid=489573115\r\n";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "ec1e.0");
      cerr << "!! "; via.encode(cerr); cerr << endl;
      assert(via.param(UnknownParameterType("stid")) == "489573115");
   }

   {
      TR _tr("Branch parameter 1");
      
      Data txt("=z9hG4bK" RESIP_COOKIE "jason-1--" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, ";");
      assert(bp.hasMagicCookie());
      assert(bp.getTransactionId() == "jason");

      bp.reset(bp.getTransactionId() + ".10");
      bp.encode(cerr); cerr << endl;
      assert(bp.getTransactionId() == "jason.10");

      Data o;
      {
         DataStream s(o);
         bp.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "jason.10-1--" RESIP_COOKIE "");
   }

      
   {
      TR _tr("Branch parameter 2");
      Data txt("=z9hG4bK" RESIP_COOKIE "jason.1.2.3-14--" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpc(ParameterTypes::branch, pb, ";");
      assert(bpc.hasMagicCookie());
      assert(bpc.getTransactionId() == "jason.1.2.3");

      Data o;
      {
         DataStream s(o);
         bpc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "jason.1.2.3-14--" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch parameter 3");
      Data txt("=z9hG4bK" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14--" RESIP_COOKIE "foobie");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14--" RESIP_COOKIE "foobie");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14--" RESIP_COOKIE "foobie");

      bpcc.reset("foobie");

      o.clear();
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "foobie-1--" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch parameter 4");
      Data txt("=z9hG4bK" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14--" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "3e565-ef7w-17.1.2.3");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14--" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch parameter 5 externally spiralled branch returns");
      Data txt("=z9hG4bK" RESIP_COOKIE "" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14" RESIP_COOKIE "foobie-1--" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14" RESIP_COOKIE "foobie");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "" RESIP_COOKIE "3e565-ef7w-17.1.2.3-14" RESIP_COOKIE "foobie-1--" RESIP_COOKIE "");
   }
   
   {
      TR _tr("Branch parameter 6 not one of ours");
      Data txt("=z9hG4bK" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "" RESIP_COOKIE "");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch parameter 7 empty ours");
      Data txt("=z9hG4bK" RESIP_COOKIE "-1--" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "-1--" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch parameter 8 badly formed ours");
      Data txt("=z9hG4bK" RESIP_COOKIE "------" RESIP_COOKIE "");
      ParseBuffer pb(txt.data(), txt.size());

      try
      {
         BranchParameter bpcc(ParameterTypes::branch, pb, ";");
         assert(bpcc.hasMagicCookie());
         bpcc.getTransactionId();
         assert(false);
      }
      catch (ParseBuffer::Exception &e)
      {
      }
   }

   {
      TR _tr("Branch parameter 9");

      Data txt("=z9hG4bK" RESIP_COOKIE "5b42cb698e8c6827790212ac5bdade1a-1-UEEzMjc2OA..-" RESIP_COOKIE ";rport;received=64.124.66.32");
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, ";");
      assert(bp.hasMagicCookie());
      assert(bp.getTransactionId() == "5b42cb698e8c6827790212ac5bdade1a");
      cerr << "!! " << bp.clientData() << endl;
      assert(bp.clientData() == "PA32768");
      
      bp.encode(cerr); cerr << endl;
   }

   {
      TR _tr("Branch parameter 10; magic cookie case mismatch");

      Data txt("=z9hG4bk15775865934415"); //little k
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, ";");
      assert(bp.hasMagicCookie());
      
      assert(bp.getTransactionId() == "15775865934415");
      Data enc;
      {
         DataStream ds(enc);
         bp.encode(ds);
      }
      cout << "!! " << enc << endl;

      assert(enc == "branch=z9hG4bk15775865934415");

      BranchParameter bpCopyCons(bp);
      Data encCopyCons;
      {
         DataStream ds(encCopyCons);
         bpCopyCons.encode(ds);
      }
      cout << "!! " << encCopyCons << endl;
      assert(bp == bpCopyCons);      
      assert(encCopyCons == "branch=z9hG4bk15775865934415");

      Data txt2("=z9hG4bk1577234fhg8df");
      ParseBuffer pb2(txt2.data(), txt2.size());

      BranchParameter bpAssignOp(ParameterTypes::branch, pb2, ";");      
      bpAssignOp = bp;
      
      Data encAssignOp;
      {
         DataStream ds(encAssignOp);
         bpAssignOp.encode(ds);
      }
      cout << "!! " << encAssignOp << endl;
      assert(bp == bpAssignOp);      
      assert(encAssignOp == "branch=z9hG4bk15775865934415");
   }

   {
      TR _tr("Branch testing 1");
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
      assert (!via.exists(p_rport));
      
      via.param(p_rport);
      assert (via.exists(p_rport));      
      assert (via.exists(p_rport));      
      assert (!via.param(p_rport).hasValue());
   }

   {
      TR _tr("Branch testing 2");
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;rport";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
      assert (via.exists(p_rport));
      assert (!via.param(p_rport).hasValue());
   }

   {
      TR _tr("Branch testing 3");
      char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;rport=100";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
      assert (via.exists(p_rport));
      assert (via.param(p_rport).hasValue());
      assert (via.param(p_rport).port() == 100);
   }

   {
      TR _tr("Branch testing 4 with clientData");
      Data txt("=z9hG4bK" RESIP_COOKIE "T-i-D-314-Q2xpZW50RGF0YQ..-" RESIP_COOKIE "");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "ClientData");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      // cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "T-i-D-314-Q2xpZW50RGF0YQ..-" RESIP_COOKIE "");
   }

   {
      TR _tr("Branch testing 5 with sigcomp ID");
      Data txt("=z9hG4bK" RESIP_COOKIE "T-i-D-314--PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4." RESIP_COOKIE "");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "");
      assert (bpcc.getSigcompCompartment() == "<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      bpcc.setSigcompCompartment("<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
//      cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "T-i-D-314--PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4." RESIP_COOKIE "");
   }

   {
      TR _tr("Branch testing 6 with sigcomp ID and Client Data");
      Data txt("=z9hG4bK" RESIP_COOKIE "T-i-D-314-Q2xpZW50RGF0YQ..-PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4." RESIP_COOKIE "");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, ";");
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "ClientData");
      assert (bpcc.getSigcompCompartment() == "<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      bpcc.setSigcompCompartment("<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      // cerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "T-i-D-314-Q2xpZW50RGF0YQ..-PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4." RESIP_COOKIE "");
   }

   //3329 tests
   {
      TR _tr( "Token + parameters parse test 3329 ");
      char *org = "digest;d-alg=md5";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dAlg) == "md5");
   }

   {
      TR _tr( "Token + parameters parse test");
      char *org = "digest;d-qop=verify";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dQop) == "verify");
   }

   {
      TR _tr( "Token + parameters parse test");
      char *org = "digest;d-ver=\"0000000000000000000000000000abcd\"";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dVer) == "0000000000000000000000000000abcd");
   }

   {
      TR _tr( "Test CSeqCategory 1");
      Data cseqString("1 INVITE");
      HeaderFieldValue hfv(cseqString.data(), cseqString.size());
      
      CSeqCategory str(&hfv, Headers::UNKNOWN);
      assert(str.sequence() == 1);
      assert(str.method() == INVITE);
      assert(Data::from(str) == cseqString);
   }

   {
      TR _tr( "Test CSeqCategory 2");
      Data cseqString("4294967295 INVITE");
      HeaderFieldValue hfv(cseqString.data(), cseqString.size());
      
      CSeqCategory str(&hfv, Headers::UNKNOWN);
      assert(str.sequence() == 4294967295);
      assert(str.method() == INVITE);
      assert(Data::from(str) == cseqString);
   }

   {
      TR _tr( "Test RAckCategory 1");
      Data rackString("1 2 INVITE");
      HeaderFieldValue hfv(rackString.data(), rackString.size());
      
      RAckCategory str(&hfv, Headers::UNKNOWN);
      assert(str.rSequence() == 1);
      assert(str.cSequence() == 2);
      assert(str.method() == INVITE);
      assert(Data::from(str) == rackString);
   }

   {
      TR _tr( "Test RAckCategory 2");
      Data rackString("4294967294 4294967295 INVITE");
      HeaderFieldValue hfv(rackString.data(), rackString.size());
      
      RAckCategory str(&hfv, Headers::UNKNOWN);
      assert(str.rSequence() == 4294967294);
      assert(str.cSequence() == 4294967295);
      assert(str.method() == INVITE);
      assert(Data::from(str) == rackString);
   }

   cerr << "\nTEST OK" << endl;

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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
