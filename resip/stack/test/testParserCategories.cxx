#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#define RESIP_COOKIE "-524287-"

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

bool failed = false;
#define ASSERT_EQ(lhs, rhs, desc) \
{ \
   if (!((lhs) == (rhs))) \
   { \
      std::cerr << __FILE__ << ":" << __LINE__ << " - " << (desc) << " " \
                << "\"" << (lhs) << "\" ( " << #lhs << " ) != " \
                << "\"" << (rhs) << "\" ( " << #rhs << " ) " << std::endl; \
      failed = true; \
   } \
}

resip::Data
toData(const resip::ParserCategory& p)
{
   resip::Data result;
   resip::oDataStream str(result);
   str << p;
   str.flush();
   return result;
}

template <class T>
void testParseAndWrite(const resip::Data& rawInput,
                       const resip::Data& parsedOutput)
{
   TR _tr("Test " + rawInput + " <-> " + parsedOutput);

   HeaderFieldValue hfv(rawInput.data(), rawInput.size());
   T raw(hfv, Headers::UNKNOWN);
   T rawCopy(raw);

   ASSERT_EQ(toData(raw), rawInput, "pre-parse");
   ASSERT_EQ(toData(rawCopy), rawInput, "pre-parse");

   raw.isWellFormed();
   T parsedCopy(raw);

   ASSERT_EQ(toData(raw), rawInput, "post-parse");
   ASSERT_EQ(toData(parsedCopy), rawInput, "post-parse");

   raw.markDirty();
   T dirtyCopy(raw);

   ASSERT_EQ(toData(raw), parsedOutput, "dirty");
   ASSERT_EQ(toData(dirtyCopy), parsedOutput, "dirty");

   raw = rawCopy;
   ASSERT_EQ(toData(raw), rawInput, "restored to raw");

   rawCopy.isWellFormed();
   ASSERT_EQ(toData(rawCopy), rawInput, "post-parse");
   rawCopy.markDirty();
   ASSERT_EQ(toData(rawCopy), parsedOutput, "dirty");
}


int
main(int arc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   static ExtensionParameter p_mobility_ext("mobility");
   static ExtensionParameter p_lr_ext("lr");
   static ExtensionParameter p_tag_ext("tag");
   static ExtensionParameter p_ttl_ext("ttl");

   {
      TR _tr("Test poorly formed NameAddr by construction");

      try
      {
         NameAddr test("<jason_AT_example.com@10.0.0.1;transport=TCP>");
         assert(false);
      }
      catch (ParseException& e)
      {
         resipCerr << e << endl;
      }
   }
   
   {
      TR _tr("Test copy transport param");

      NameAddr test("<sip:jason_AT_example.com@10.0.0.1:5060;transport=TCP>");
      resipCerr << test << endl;
      NameAddr copy = test;
      resipCerr << copy << endl;
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
      TR _tr("Test NameAddr encode from underlying after read-only parse");

      resip::Data raw("<sip:jason_AT_example.com@10.0.0.1:5060;opaque=blah>");
      HeaderFieldValue hfv(raw.data(), raw.size());
      NameAddr test(hfv, Headers::UNKNOWN);
      const NameAddr& c_test(test);
      // We should be calling the const version of uri() here, since we don't 
      // need to modify anything.
      static ExtensionParameter p_opaque_ext("opaque");
      resip::Data opaque(c_test.uri().param(p_opaque_ext));
      resipCerr << test << endl;
      NameAddr copy = test;
      resipCerr << copy << endl;
      assert(resip::Data::from(test)==raw);
      assert(resip::Data::from(copy)==raw);
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
            resipCerr << res << endl;
            
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
      resipCerr << uri2.param(UnknownParameterType("a")) << endl;
      
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

      resipCerr << "!!" << legal << endl;

      assert(legal.displayName() == "A. Bell");
   }

   {
      TR _tr("Test quoted displayname in NameAddr (torture test: 2.39)");
      Data data("\"A. Bell\" <sip:a.g.bell@bell-tel.com>;tag=459843");

      NameAddr legal(data);

      assert(legal.uri().host() == "bell-tel.com");

      resipCerr << "!!" << legal.displayName() << endl;

      assert(legal.displayName() == "A. Bell");
   }

   {
      TR _tr("Test NameAddr parameter handling");
      Data data("sip:foo@bar.com;user=phone");
      
      NameAddr original(data);
      assert(original.uri().exists(p_user));
      
      resipCerr << "!!" << original << endl;
   }

   {
      TR _tr("Test tel aor canonicalization");
      Data data("tel:+14156268178;pOstd=pP2;isUb=1411");
      
      Uri original(data);
      resipCerr << original.getAor() << endl;
      
      assert(original.getAor() == "+14156268178");
   }

   {
      TR _tr("Test aor canonicalization");
      Data data("sip:User@kElOwNa.GlOo.NeT:5666");
      Data data1("sip:User@KeLoWnA.gLoO.nEt:5666");
      
      Uri original(data);
      Uri original1(data1);

      resipCerr << "!! " << original.getAor() << " " << original1.getAor() << endl;
      assert(original.getAor() == original1.getAor());
   }

   {
      TR _tr("Test tel NameAddr");
      NameAddr n1("<tel:98267168>");
      resipCerr << n1.uri().user() << endl;
   }

#ifdef USE_IPV6
   {
      TR _tr("Test cleverly malformed V6 addr in Uri");
      const char* buf="sip:foo@[:x]";
      HeaderFieldValue hfv(buf, strlen(buf));
      NameAddr nameaddr(hfv, Headers::UNKNOWN);
      assert(!nameaddr.isWellFormed());
   }
#endif
   
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
      Auth auth(hfv, Headers::Authorization);
      
      assert(auth.scheme() == "Digest");
      assert(auth.exists(p_realm));
   }

   {
      TR _tr("Test uri with no user");
      Data data("sip:kelowna.gloo.net");
      
      Uri original(data);
      resipCerr << original << endl;
      
      assert(Data::from(original) == data);
   }

   {
      TR _tr("Test uri with empty transport param");
      Data data("sip:kelowna.gloo.net;transport=;udp");
      
      try
      {
         Uri original(data);
         assert(0);
      }
      catch(...)
      {}
   }

   {
      TR _tr("Test uri with empty transport param");
      Data data("sip:kelowna.gloo.net;transport=;udp");
      
      try
      {
         Uri original(data);
         assert(0);
      }
      catch(...)
      {}
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
      TR _tr("Test @ in params for Uri");
      Uri uri(Data("sip:example.com;lr;foo=\"@haha\""));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted @ in params for Uri");
      // This is actually not an ambiguous case; '@' is not valid in either a 
      // param name or value, and ';' is legal in a userpart
      Uri uri(Data("sip:example.com;lr;foo=fooba@haha"));
      assert(uri.user()=="example.com;lr;foo=fooba");
      assert(uri.host()=="haha");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test : in params for Uri");
      Uri uri(Data("sip:example.com;lr;foo=\":lol\""));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted : in params for Uri");
      Uri uri(Data("sip:example.com;lr;foo=:lol"));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted : in params for Uri (2)");
      Uri uri(Data("sip:example.com;lr;:foo=:lol"));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test @ in headers for Uri");
      Uri uri(Data("sip:example.com?foo=\"@haha\""));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted @ in headers for Uri");
      // This is actually not an ambiguous case; '@' is not valid in either an 
      // embedded header name or value, and '?' is legal in a userpart
      Uri uri(Data("sip:example.com?foo=fooba@haha"));
      assert(uri.user()=="example.com?foo=fooba");
      assert(uri.host()=="haha");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test : in headers for Uri");
      Uri uri(Data("sip:example.com?foo=\":lol\""));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted : in headers for Uri");
      Uri uri(Data("sip:example.com?foo=:lol"));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test unquoted : in headers for Uri (2)");
      Uri uri(Data("sip:example.com?:foo=:lol"));
      assert(uri.user().empty());
      assert(uri.host()=="example.com");
      assert(uri.port()==0);
   }

   {
      TR _tr("Test typeless parameter copy");
      Token s = Token("jason");
      s.value() = "value";
      s.param(p_expires) = 17;
      s.param(p_lr_ext);
      s.param(UnknownParameterType("foobie")) = "quux";

      Token s1;
      s1.value() = "other";
      s1.param(p_retryAfter) = 21;

      s.setParameter(s1.getParameterByEnum(ParameterTypes::retryAfter));
      assert(s.value() == "value");
      assert(s.param(p_retryAfter) == 21);
      assert(s.exists(p_lr_ext));
      assert(s.param(UnknownParameterType("foobie")) == "quux");
   }

   {
      TR a("Test typeless parameter overwrite" );
      Token s;
      s.value() = "value";
      s.param(p_expires) = 17;
      s.param(p_retryAfter) = 12;
      s.param(p_lr_ext);
      s.param(UnknownParameterType("foobie")) = "quux";

      Token s1;
      s1.value() = "other";
      s1.param(p_retryAfter) = 21;

      s.setParameter(s1.getParameterByEnum(ParameterTypes::retryAfter));
      assert(s.value() == "value");
      assert(s.param(p_retryAfter) == 21);
      assert(s.exists(p_lr_ext));
      assert(s.param(UnknownParameterType("foobie")) == "quux");

      s.encode(resipCerr);
      resipCerr << endl;
   }

   {
      TR _tr( "Test StringCategory");
      Data stringString("Lame Agent");
      HeaderFieldValue hfv(stringString.data(), stringString.size());
      
      StringCategory str(hfv, Headers::UNKNOWN);
      assert(str.value() == stringString);

      Data buff;
      {
         DataStream s(buff);
         str.encode(s);
      }
      resipCerr << buff << endl;
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
      resipCerr << state << endl;
   }

   {
      TR _tr( "StatusLine, with reason code");
      Data statusLineString("SIP/2.0 180 Ringing");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(hfv);
      assert(statusLine.responseCode() == 180);
      resipCerr << statusLine.reason() << endl;
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

#define checkHeaderName(_name) resipCerr << Headers::_name << " " << Headers::getHeaderName(Headers::_name) << " = " << #_name << endl /*;assert(isEqualNoCase(Headers::getHeaderName(Headers::_name), #_name))*/
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
      checkHeaderName(PAccessNetworkInfo);
      checkHeaderName(PChargingVector);
      checkHeaderName(PChargingFunctionAddresses);
      checkHeaderName(PVisitedNetworkID);
      checkHeaderName(UserToUser);
   }

#define checkParameterName(_name) resipCerr << ParameterTypes::_name << " " << ParameterTypes::ParameterNames[ParameterTypes::_name] << " = " << #_name << endl/*;assert(isEqualNoCase(ParameterTypes::ParameterNames[ParameterTypes::_name], #_name))*/
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
      checkParameterName(gr);

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
      
      checkParameterName(utranCellId3gpp);
      checkParameterName(cgi3gpp);
      checkParameterName(ccf);
      checkParameterName(ecf);
      checkParameterName(icidValue);
      checkParameterName(icidGeneratedAt);
      checkParameterName(origIoi);
      checkParameterName(termIoi);
      //checkParameterName(purpose);
      checkParameterName(content);
      checkParameterName(encoding);

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
      const char *org = "WuggaWuggaFoo";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(hfv, Headers::UNKNOWN);
      assert(tok.value() == org);
   }

   {
      TR _tr( "Token + parameters parse test");
      const char *org = "WuggaWuggaFoo;ttl=2;retry-after=3";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(hfv, Headers::UNKNOWN);
      assert(tok.value() == "WuggaWuggaFoo");
      assert(tok.param(p_ttl_ext) == "2");
      assert(tok.param(p_retryAfter) == 3);
   }

   {
      TR _tr( "Test NameAddr(Data) constructor");
      
      Data nad("bob<sips:bob@foo.com>;tag=wd834f");
      NameAddr na(nad); 
      assert(na.uri().user() == "bob");
   }

   {
      TR _tr( "full on via parse");
      const char *viaString = /* Via: */ " SIP/2.0/UDP a.b.c.com:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "a.b.c.com");
      assert(via.param(p_maddr) == "1.2.3.4");
   }

#ifdef USE_IPV6
   {
      TR _tr( "Via assert bug with malformed IPV6 addr [boom]" );
      const char* viaString = "SIP/2.0/UDP [boom]:5060;branch=z9hG4bKblah";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      assert(!via.isWellFormed());
   }

   {
      TR _tr( "Via assert bug with malformed IPV6 addr [:z]" );
      const char* viaString = "SIP/2.0/UDP [:z]:5060;branch=z9hG4bKblah";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      assert(!via.isWellFormed());
   }
#endif

   {
      TR _tr("Test poorly formed DataParameter by construction");

      const char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;tag=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseException& e)
      {
         resipCerr << "Caught parse exception for Via" << endl;
      }
   }

   {
      TR _tr("Test poorly formed UnknownParameter by construction");

      const char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;foobar=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(hfv, Headers::UNKNOWN);
         via.sentPort();
         UnknownParameterType p_foobar("foobar");
         via.exists(p_foobar);
         assert(false);
      }
      catch (ParseException& e)
      {
         resipCerr << "Caught parse exception for Via" << endl;
      }
   }

   {
      TR _tr("Test poorly formed UInt32Parameter by construction");

      const char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;duration=";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseException& e)
      {
         resipCerr << "Caught parse exception for Via " << endl;
      }
   }

   {
      TR _tr("Test poorly formed QuotedDataParameter by construction");

      const char *viaString = /* Via: */ " SIP/2.0/UDP example.com:5000;;domain=\"";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      try
      {
         Via via(hfv, Headers::UNKNOWN);
         via.sentPort();
         assert(false);
      }
      catch (ParseException& e)
      {
         resipCerr << "Caught parse exception for Via " << endl;
      }
   }

#ifdef USE_IPV6
   {
      TR _tr( "full on via parse, IPV6");
      // !dlb! deal with maddr=[5f1b:df00:ce3e:e200:20:800:2b37:6426]
      const char *viaString = /* Via: */ " SIP/2.0/UDP [5f1b:df00:ce3e:e200:20:800:2b37:6426]:5000;ttl=3;maddr=1.2.3.4;received=foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
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
      resipCerr << "!! " << to << endl;
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
      resipCerr << "Uri:" << uri.host() << endl;
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
   
   resipCerr << "URI comparison tests" << endl;
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
      resipCerr << "A: " << uA << endl;
      resipCerr << "B: " << uB << endl;
      resipCerr << "A:exists(transport) " << uA.exists(p_transport) << endl;
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

      RequestLine requestLine(hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      resipCerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "Request Line parse tel");
      Data requestLineString("INVITE tel:4153331212 SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(hfv);
      assert(requestLine.uri().scheme() == "tel");
      assert(requestLine.uri().user() == "4153331212");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "Request Line parse, parameters");
      Data requestLineString("INVITE sips:bob@foo.com;maddr=1.2.3.4 SIP/2.0");
      HeaderFieldValue hfv(requestLineString.data(), requestLineString.size());

      RequestLine requestLine(hfv);
      assert(requestLine.uri().scheme() == "sips");
      assert(requestLine.uri().user() == "bob");
      resipCerr << requestLine.uri().host() << endl;
      assert(requestLine.uri().host() == "foo.com");
      assert(requestLine.getMethod() == INVITE);
      assert(requestLine.uri().param(p_maddr) == "1.2.3.4");
      resipCerr << requestLine.getSipVersion() << endl;
      assert(requestLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "NameAddr parse");
      Data nameAddrString("sips:bob@foo.com");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, displayName");
      Data nameAddrString("Bob<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, quoted displayname");
      Data nameAddrString = "\"Bob\"<sips:bob@foo.com>";
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, quoted displayname, embedded quotes");
      Data nameAddrString("\"Bob   \\\" asd   \"<sips:bob@foo.com>");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob   \\\" asd   ");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");
      assert(nameAddr.uri().host() == "foo.com");
   }
   {
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com>;tag=456248;mobility=\"hobble\"");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      resipCerr << "Uri params: ";
      nameAddr.uri().encodeParameters(resipCerr) << endl;
      resipCerr << "Header params: ";
      nameAddr.encodeParameters(resipCerr) << endl;
      assert(nameAddr.param(p_tag) == "456248");
      assert(nameAddr.param(p_mobility) == "hobble");

      assert(nameAddr.uri().exists(p_tag_ext) == false);
      assert(nameAddr.uri().exists(p_mobility_ext) == false);
   }
   {
      TR _tr( "NameAddr parse, quoted displayname, parameterMove");
      Data nameAddrString("\"Bob\"<sips:bob@foo.com>;tag=456248;mobility=\"hobble\"");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      resipCerr << "Uri params: ";
      nameAddr.uri().encodeParameters(resipCerr) << endl;
      resipCerr << "Header params: ";
      nameAddr.encodeParameters(resipCerr) << endl;

      assert(nameAddr.param(p_tag) == "456248");
      assert(nameAddr.param(p_mobility) == "hobble");

      assert(nameAddr.uri().exists(p_tag_ext) == false);
      assert(nameAddr.uri().exists(p_mobility_ext) == false);
   }
   {
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com;tag=456248;mobility=\"hobble\">");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      resipCerr << "Uri params: ";
      nameAddr.uri().encodeParameters(resipCerr) << endl;
      resipCerr << "Header params: ";
      nameAddr.encodeParameters(resipCerr) << endl;
      assert(nameAddr.uri().param(p_tag_ext) == "456248");
      assert(nameAddr.uri().param(p_mobility_ext) == "hobble");

      assert(nameAddr.exists(p_tag) == false);
      assert(nameAddr.exists(p_mobility) == false);
   }
   {
      TR _tr( "NameAddr parse, unquoted displayname, paramterMove");
      Data nameAddrString("Bob<sips:bob@foo.com;mobility=\"hobb;le\";tag=\"true;false\">");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());

      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "Bob");
      assert(nameAddr.uri().scheme() == "sips");
      assert(nameAddr.uri().user() == "bob");

      assert(nameAddr.uri().host() == "foo.com");
      
      resipCerr << "Uri params: ";
      nameAddr.uri().encodeParameters(resipCerr) << endl;
      resipCerr << "Header params: ";
      nameAddr.encodeParameters(resipCerr) << endl;
      assert(nameAddr.uri().param(p_mobility_ext) == "hobb;le");
      assert(nameAddr.uri().param(p_tag_ext) == "true;false");
      //      assert("true;false" == nameAddr.uri().param(Data("useless")));

      assert(nameAddr.exists(p_mobility) == false);
   }
   {
      TR _tr( "NameAddr parse");
      Data nameAddrString("sip:101@localhost:5080;transport=UDP");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(hfv, Headers::UNKNOWN);
      assert(nameAddr.displayName() == "");
      assert(nameAddr.uri().scheme() == "sip");
      assert(nameAddr.uri().user() == "101");
   }
   {
      TR _tr( "NameAddr parse, no user in uri");
      Data nameAddrString("sip:localhost:5070");
      HeaderFieldValue hfv(nameAddrString.data(), nameAddrString.size());
      
      NameAddr nameAddr(hfv, Headers::UNKNOWN);
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
      
      StatusLine statusLine(hfv);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr( "StatusLine, no reason code");
      Data statusLineString("SIP/2.0 100");
      HeaderFieldValue hfv(statusLineString.data(), statusLineString.size());
      
      StatusLine statusLine(hfv);
      assert(statusLine.responseCode() == 100);
      assert(statusLine.reason() == "");
      assert(statusLine.getSipVersion() == "SIP/2.0");
   }
   {
      TR _tr("Auth Schemes");
      const char* authorizationString = "Digest realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(hfv, Headers::UNKNOWN);

      resipCerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "Digest");
      resipCerr << "   realm: " <<  auth.param(p_realm) << endl;
      assert(auth.param(p_realm) == "66.100.107.120"); 
      assert(auth.param(p_username) == "1234"); 
      assert(auth.param(p_nonce) == "1011235448"); 
      assert(auth.param(p_uri) == "sip:66.100.107.120"); 
      assert(auth.param(p_algorithm) == "MD5"); 
      assert(auth.param(p_response) == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      Data dsData;
      {
         DataStream s(dsData);
         auth.encode(s);
      }

      resipCerr << dsData.c_str() << endl;
      
      assert(dsData == "Digest realm=\"66.100.107.120\",username=\"1234\",nonce=\"1011235448\",uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }

   {
      TR _tr("Auth Missing Algorithm Type Param Name");  // Reported infinite loop on mailing list - fixed on Oct 17, 2014
      const char* authorizationString = "Digest username=\"000999234\",realm=\"1.1.1.1\",nonce=\"1413544408:b15ee1a80dd75f9db443e2d4feab821b\",uri=\"sip:1.1.1.1\",=MD5,response=\"ef0f8cdc6a75fe810e2ce82a2758f45e\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(hfv, Headers::UNKNOWN);

      resipCerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "Digest");
      resipCerr << "   realm: " <<  auth.param(p_realm) << endl;
      assert(auth.param(p_realm) == "1.1.1.1"); 
      assert(auth.param(p_username) == "000999234"); 
      assert(auth.param(p_nonce) == "1413544408:b15ee1a80dd75f9db443e2d4feab821b"); 
      assert(auth.param(p_uri) == "sip:1.1.1.1"); 
      //assert(auth.param(p_algorithm) == "MD5"); 
      assert(auth.param(p_response) == "ef0f8cdc6a75fe810e2ce82a2758f45e"); 

      Data dsData;
      {
         DataStream s(dsData);
         auth.encode(s);
      }

      resipCerr << dsData.c_str() << endl;
      
      assert(dsData == "Digest username=\"000999234\",realm=\"1.1.1.1\",nonce=\"1413544408:b15ee1a80dd75f9db443e2d4feab821b\",uri=\"sip:1.1.1.1\",response=\"ef0f8cdc6a75fe810e2ce82a2758f45e\"");
   }

   {
      TR _tr("More Auth");
      const char* authorizationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\"";
      HeaderFieldValue hfv(authorizationString, strlen(authorizationString));
      
      Auth auth(hfv, Headers::UNKNOWN);

      resipCerr << "Auth scheme: " <<  auth.scheme() << endl;
      assert(auth.scheme() == "");
      resipCerr << "   realm: " <<  auth.param(p_realm) << endl;
      assert(auth.param(p_realm) == "66.100.107.120"); 
      assert(auth.param(p_username) == "1234"); 
      assert(auth.param(p_nonce) == "1011235448"); 
      assert(auth.param(p_uri) == "sip:66.100.107.120"); 
      assert(auth.param(p_algorithm) == "MD5"); 
      assert(auth.param(p_response) == "8a5165b024fda362ed9c1e29a7af0ef2"); 

      Data dsData;
      {
         DataStream s(dsData);
         auth.encode(s);
      }

      resipCerr << dsData.c_str() << endl;
      
      assert(dsData == "realm=\"66.100.107.120\",username=\"1234\",nonce=\"1011235448\",uri=\"sip:66.100.107.120\",algorithm=MD5,response=\"8a5165b024fda362ed9c1e29a7af0ef2\"");
   }
   
   {
      TR _tr("Testing qop stuff");
      const char* authenticationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, qop=\"auth,auth-int\"";
      const char* authorizationString = "realm=\"66.100.107.120\", username=\"1234\", nonce=\"1011235448\"   , uri=\"sip:66.100.107.120\"   , algorithm=MD5, response=\"8a5165b024fda362ed9c1e29a7af0ef2\", qop=auth";
      HeaderFieldValue authenHfv(authenticationString, strlen(authenticationString));
      HeaderFieldValue authorHfv(authorizationString, strlen(authorizationString));
      
      Auth wwwAuthen(authenHfv, Headers::WWWAuthenticate);
      Auth pAuthen(authenHfv, Headers::ProxyAuthenticate);
      Auth authInfo(authorHfv, Headers::AuthenticationInfo);
      Auth pAuthor(authorHfv, Headers::ProxyAuthorization);
      Auth author(authorHfv, Headers::Authorization);

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

      const char* genericString = "<http://www.google.com>;purpose=icon;fake=true";
      HeaderFieldValue hfv(genericString, strlen(genericString));

      GenericUri generic(hfv, Headers::UNKNOWN);

      assert(generic.uri() == "http://www.google.com");
      resipCerr << generic.param(p_purpose) << endl;
      assert(generic.param(p_purpose) == "icon");
      assert(generic.param(UnknownParameterType("fake")) == "true");

      Data dsData;
      {
         DataStream s(dsData);
         generic.encode(s);
      }

      resipCerr << dsData.c_str() << endl;
      
      assert(dsData == "<http://www.google.com>;purpose=icon;fake=true");
   }

   {
      TR _tr("Date testing 1");
      const char *dateString = "Mon, 04 Nov 2002 17:34:15 GMT";
      HeaderFieldValue hfv(dateString, strlen(dateString));
      
      DateCategory date(hfv, Headers::UNKNOWN);

      assert(date.dayOfWeek() == Mon);
      assert(date.dayOfMonth() == 04);
      assert(date.month() == Nov);
      assert(date.year() == 2002);
      assert(date.hour() == 17);
      assert(date.minute() == 34);
      assert(date.second() == 15);

      Data dsData;
      {
         DataStream s(dsData);
         date.encode(s);
      }

      resipCerr << dsData.c_str() << endl;

      assert(dsData == dateString);
      
      // copy ctor  not working in v1.94 ParserCategories.cxx
      
      dsData.clear();
      
      DateCategory otherDate(date);
      {
         DataStream s2(dsData);
         otherDate.encode(s2);
      }
      resipCerr << "!! original date     : " << date << endl;
      resipCerr << "!! original string   : " << dateString << endl;
      resipCerr << "!! otherDate         : " << otherDate << endl;
      resipCerr << "!! encoded otherDate : " <<  dsData.c_str() << endl;
      assert (dsData == dateString);

   }

   {
      TR _tr("Date testing 2");
      const char *dateString = "  Sun  , 14    Jan 2222 07:04:05   GMT    ";
      HeaderFieldValue hfv(dateString, strlen(dateString));
      
      DateCategory date(hfv, Headers::UNKNOWN);

      assert(date.dayOfWeek() == Sun);
      assert(date.dayOfMonth() == 14);
      assert(date.month() == Jan);
      assert(date.year() == 2222);
      assert(date.hour() == 07);
      assert(date.minute() == 04);
      assert(date.second() == 05);

      Data dsData;
      {
         DataStream s(dsData);
         date.encode(s);
      }
      assert(dsData == "Sun, 14 Jan 2222 07:04:05 GMT");
   }


   {
      TR _tr("Mime types 1");

      const char* mimeString = "application/sdp";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(hfv, Headers::UNKNOWN);

      assert(mime.type() == "application");
      assert(mime.subType() == "sdp");

      Data dsData;
      {
         DataStream s(dsData);
         mime.encode(s);
      }
      assert(dsData == mimeString);
   }


   {
      TR _tr("Mime types 2");
      const char* mimeString = "text/html ; charset=ISO-8859-4";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(hfv, Headers::UNKNOWN);

      assert(mime.type() == "text");
      assert(mime.subType() == "html");
      assert(mime.param(p_charset) == "ISO-8859-4");

      Data dsData;
      {
         DataStream s(dsData);
         mime.encode(s);
      }
      assert(dsData == "text/html;charset=ISO-8859-4");
   }

   {
      TR _tr("Mime types 3");

      const char* mimeString = "    text   /     html        ;  charset=ISO-8859-4";
      HeaderFieldValue hfv(mimeString, strlen(mimeString));
      
      Mime mime(hfv, Headers::UNKNOWN);

      assert(mime.type() == "text");
      assert(mime.subType() == "html");
      assert(mime.param(p_charset) == "ISO-8859-4");

      Data dsData;
      {
         DataStream s(dsData);
         mime.encode(s);
      }
      assert(dsData == "text/html;charset=ISO-8859-4");
   }

   {
      TR _tr("Via 1");

      Via via;
      via.encode(resipCerr);
      resipCerr << endl;

      assert (via.param(p_branch).hasMagicCookie());
   }

   {
      TR _tr("Via 2");
      
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());

      Data dsData;
      {
         DataStream s0(dsData);
         via.encode(s0);
      }
      resipCerr << dsData.c_str() << endl;
      assert(dsData == "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj");
      
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      
      dsData.clear();
      {
         DataStream s1(dsData);
         via.encode(s1);
      }
      assert(dsData == "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj");
      
      via.param(p_branch).reset("jason");
      dsData.clear();
      {
         DataStream s2(dsData);
         via.encode(s2);
      }
      
      resipCerr << "!! " << dsData.c_str() << endl;
      assert(dsData == "SIP/2.0/UDP ;branch=z9hG4bK" RESIP_COOKIE "1---jason");
      assert(via.param(p_branch).getTransactionId() == "jason");
   }

   {
      TR _tr("Via 3");
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
   }

   {
      TR _tr("Via 4");
      const char* viaString = "SIP/2.0/UDP ;branch=oldassbranch";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (!via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "oldassbranch");
      
      Data dsData;
      DataStream s(dsData);
      via.encode(s);
      s.flush();
      assert(dsData == "SIP/2.0/UDP ;branch=oldassbranch");
      
      via.param(p_branch).reset("jason");
      dsData.clear();
      via.encode(s);
      s.flush();
      assert(dsData == "SIP/2.0/UDP ;branch=z9hG4bK" RESIP_COOKIE "1---jason");
      assert(via.param(p_branch).getTransactionId() == "jason");
   }

   {
      TR _tr("Via 5 assignment with unknown parameter");
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;stid=abcd.2";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);

      Via via1;
      via1 = via;

      resipCerr << "!! "; via1.encode(resipCerr); resipCerr << endl;
      assert(via1.param(UnknownParameterType("stid")) == "abcd.2");
   }

   {
      TR _tr("Via 6 parse with known parameter");
      const char* viaString = "SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "1---ec1e.0;ttl=4\r\n";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "ec1e.0");
      resipCerr << "!! "; via.encode(resipCerr); resipCerr << endl;
      assert(via.param(p_ttl) == 4);
   }

   {
      TR _tr("Via 7 parse with unknown parameter");
      const char* viaString = "SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "1---ec1e.0;stid=489573115\r\n";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "ec1e.0");
      resipCerr << "!! "; via.encode(resipCerr); resipCerr << endl;
      assert(via.param(UnknownParameterType("stid")) == "489573115");
   }

   {
      TR _tr("Branch parameter 1");
      
      Data txt("=z9hG4bK" RESIP_COOKIE "1---jason");
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bp.hasMagicCookie());
      assert(bp.getTransactionId() == "jason");

      bp.reset(bp.getTransactionId() + ".10");
      bp.encode(resipCerr); resipCerr << endl;
      assert(bp.getTransactionId() == "jason.10");

      Data o;
      {
         DataStream s(o);
         bp.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "1---jason.10");
   }

      
   {
      TR _tr("Branch parameter 2");
      Data txt("=z9hG4bK" RESIP_COOKIE "14---jason.1.2.3");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bpc.hasMagicCookie());
      assert(bpc.getTransactionId() == "jason.1.2.3");

      Data o;
      {
         DataStream s(o);
         bpc.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "14---jason.1.2.3");
   }

   {
      TR _tr("Branch parameter 3");
      Data txt("=z9hG4bK" RESIP_COOKIE "14---3e565-ef7w-17.1.2.3foobie");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "3e565-ef7w-17.1.2.3foobie");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "14---3e565-ef7w-17.1.2.3foobie");

      bpcc.reset("foobie");

      o.clear();
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "1---foobie");
   }

   {
      TR _tr("Branch parameter 4");
      Data txt("=z9hG4bK" RESIP_COOKIE "14---3e565-ef7w-17.1.2.3");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "3e565-ef7w-17.1.2.3");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "14---3e565-ef7w-17.1.2.3");
   }

   {
      TR _tr("Branch parameter 7 empty ours");
      Data txt("=z9hG4bK" RESIP_COOKIE "1---");
      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bpcc.hasMagicCookie());
      assert(bpcc.getTransactionId() == "");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "1---");
   }

   {
      TR _tr("Branch parameter 8 badly formed ours");
      Data txt("=z9hG4bK" RESIP_COOKIE "------");
      ParseBuffer pb(txt.data(), txt.size());

      try
      {
         BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
         assert(bpcc.hasMagicCookie());
         bpcc.getTransactionId();
         assert(false);
      }
      catch (ParseException &e)
      {
      }
   }

   {
      TR _tr("Branch parameter 9");

      Data txt("=z9hG4bK" RESIP_COOKIE "1-UEEzMjc2OA..--5b42cb698e8c6827790212ac5bdade1a;rport;received=64.124.66.32");
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert(bp.hasMagicCookie());
      assert(bp.getTransactionId() == "5b42cb698e8c6827790212ac5bdade1a");
      resipCerr << "!! " << bp.clientData() << endl;
      assert(bp.clientData() == "PA32768");
      
      bp.encode(resipCerr); resipCerr << endl;
   }

   {
      TR _tr("Branch parameter 10; magic cookie case mismatch");

      Data txt("=z9hG4bk15775865934415"); //little k
      ParseBuffer pb(txt.data(), txt.size());
      BranchParameter bp(ParameterTypes::branch, pb, Data::toBitset(";"));
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

      BranchParameter bpAssignOp(ParameterTypes::branch, pb2, Data::toBitset(";"));
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
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
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
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;rport";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
      assert (via.exists(p_rport));
      assert (!via.param(p_rport).hasValue());
   }

   {
      TR _tr("Branch testing 3");
      const char* viaString = "SIP/2.0/UDP ;branch=z9hG4bKwkl3lkjsdfjklsdjklfdsjlkdklj ;ttl=70;rport=100";
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(hfv, Headers::UNKNOWN);
      
      assert (via.param(p_branch).hasMagicCookie());
      assert (via.param(p_branch).getTransactionId() == "wkl3lkjsdfjklsdjklfdsjlkdklj");
      assert (via.param(p_ttl) == 70);
      assert (via.exists(p_rport));
      assert (via.param(p_rport).hasValue());
      assert (via.param(p_rport).port() == 100);
   }

   {
      TR _tr("Branch testing 4 with clientData");
      Data txt("=z9hG4bK" RESIP_COOKIE "314-Q2xpZW50RGF0YQ..--T-i-D");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "ClientData");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      // resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "314-Q2xpZW50RGF0YQ..--T-i-D");
   }

   {
      TR _tr("Branch testing 5 with sigcomp ID");
      Data txt("=z9hG4bK" RESIP_COOKIE "314--PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4.-T-i-D");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "");
      assert (bpcc.getSigcompCompartment() == "<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      bpcc.setSigcompCompartment("<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
//      resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "314--PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4.-T-i-D");
   }

   {
      TR _tr("Branch testing 6 with sigcomp ID and Client Data");
      Data txt("=z9hG4bK" RESIP_COOKIE "314-Q2xpZW50RGF0YQ..-PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4.-T-i-D");

      ParseBuffer pb(txt.data(), txt.size());

      BranchParameter bpcc(ParameterTypes::branch, pb, Data::toBitset(";"));
      assert (bpcc.getTransactionId() == "T-i-D");
      assert (bpcc.clientData() == "ClientData");
      assert (bpcc.getSigcompCompartment() == "<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      bpcc.setSigcompCompartment("<urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>");

      Data o;
      {
         DataStream s(o);
         bpcc.encode(s);
      }
      // resipCerr << "!! " << o << endl;
      assert(o == "branch=z9hG4bK" RESIP_COOKIE "314-Q2xpZW50RGF0YQ..-PHVybjp1dWlkOmZhMzNjNzJkLTEyMWYtNDdlOC00MmUyLTFlYjZlMjRhYmE2ND4.-T-i-D");
   }

   //3329 tests
   {
      TR _tr( "Token + parameters parse test 3329 ");
      const char *org = "digest;d-alg=md5";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dAlg) == "md5");
   }

   {
      TR _tr( "Token + parameters parse test");
      const char *org = "digest;d-qop=verify";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dQop) == "verify");
   }

   {
      TR _tr( "Token + parameters parse test");
      const char *org = "digest;d-ver=\"0000000000000000000000000000abcd\"";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(hfv, Headers::UNKNOWN);
      assert(tok.value() == "digest");
      assert(tok.param(p_dVer) == "0000000000000000000000000000abcd");
   }

   {
      TR _tr( "Test CSeqCategory 1");
      Data cseqString("1 INVITE");
      HeaderFieldValue hfv(cseqString.data(), cseqString.size());
      
      CSeqCategory str(hfv, Headers::UNKNOWN);
      assert(str.sequence() == 1);
      assert(str.method() == INVITE);
      assert(Data::from(str) == cseqString);
   }

   {
      TR _tr( "Test CSeqCategory 2");
      Data cseqString("4294967295 INVITE");
      HeaderFieldValue hfv(cseqString.data(), cseqString.size());
      
      CSeqCategory str(hfv, Headers::UNKNOWN);
      assert(str.sequence() == 4294967295U);
      assert(str.method() == INVITE);
      assert(Data::from(str) == cseqString);
   }

   {
      TR _tr( "Test RAckCategory 1");
      Data rackString("1 2 INVITE");
      HeaderFieldValue hfv(rackString.data(), rackString.size());
      
      RAckCategory str(hfv, Headers::UNKNOWN);
      assert(str.rSequence() == 1);
      assert(str.cSequence() == 2);
      assert(str.method() == INVITE);
      assert(Data::from(str) == rackString);
   }

   {
      TR _tr( "Test RAckCategory 2");
      Data rackString("4294967294 4294967295 INVITE");
      HeaderFieldValue hfv(rackString.data(), rackString.size());
      
      RAckCategory str(hfv, Headers::UNKNOWN);
      assert(str.rSequence() == 4294967294U);
      assert(str.cSequence() == 4294967295U);
      assert(str.method() == INVITE);
      assert(Data::from(str) == rackString);
   }

   {
      TR _tr( "Test Privacy Category 1");
      Data privacy("user;session;critical");
      HeaderFieldValue hfv(privacy.data(), privacy.size());
      
      PrivacyCategory test(hfv, Headers::UNKNOWN);
      assert(test.value().size()==3);
      assert(test.value().back()=="critical");
      test.value().pop_back();
      assert(test.value().back()=="session");
      test.value().pop_back();
      assert(test.value().back()=="user");
      test.value().pop_back();
   }

   {
      TR _tr( "Test Privacy Category 2");
      Data privacy("user; session;   critical  ");
      HeaderFieldValue hfv(privacy.data(), privacy.size());
      
      PrivacyCategory test(hfv, Headers::UNKNOWN);
      assert(test.value().size()==3);
      assert(test.value().back()=="critical");
      test.value().pop_back();
      assert(test.value().back()=="session");
      test.value().pop_back();
      assert(test.value().back()=="user");
      test.value().pop_back();
   }

   {
      TR _tr( "Test Privacy Category 3");
      Data privacy("user");
      HeaderFieldValue hfv(privacy.data(), privacy.size());
      
      PrivacyCategory test(hfv, Headers::UNKNOWN);
      assert(test.value().size()==1);
      assert(test.value().back()=="user");
      test.value().pop_back();
   }

   testParseAndWrite<NameAddr>("*", "*");
   testParseAndWrite<NameAddr>(" *", "*");
   testParseAndWrite<NameAddr>("* ", "*");
   testParseAndWrite<NameAddr>("* ;foo", "*;foo");
   testParseAndWrite<NameAddr>("*bob*<sip:bob@foo>", "\"*bob*\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("\"bob\"<sip:bob@foo>", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("bob<sip:bob@foo> ", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("*bob* <sip:bob@foo>", "\"*bob*\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("\"bob\" <sip:bob@foo>", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("bob <sip:bob@foo> ", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("bob < sip:bob@foo> ", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>(" bob<sip:bob@foo>", "\"bob\"<sip:bob@foo>");
   testParseAndWrite<NameAddr>("sip:bob@foo", "<sip:bob@foo>");
   testParseAndWrite<NameAddr>("sip:bob@foo;foo", "<sip:bob@foo>;foo");
   testParseAndWrite<NameAddr>("sip:bob@foo; foo", "<sip:bob@foo>;foo");
   testParseAndWrite<NameAddr>("sip:bob@foo;foo ", "<sip:bob@foo>;foo");

#ifdef USE_IPV6
   testParseAndWrite<Via>("SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]", "SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]");
   testParseAndWrite<Via>("SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5];branch=first", "SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5];branch=first");
   testParseAndWrite<Via>("SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=first", "SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=first");
   testParseAndWrite<Via>("SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=z9hG4bK", "SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=z9hG4bK");
   testParseAndWrite<Via>("SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=Z9Hg4Bk", "SIP/2.0/UDP [fe80::5626:96ff:fed5:c1f5]:5060;branch=Z9Hg4Bk");
#endif
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com", "SIP/2.0/UDP biloxi.com");
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com;branch=first", "SIP/2.0/UDP biloxi.com;branch=first");
   testParseAndWrite<Via>("sip/2.0/udp biloxi.com:5060;branch=first", "sip/2.0/udp biloxi.com:5060;branch=first");
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com:5060;branch=z9hG4bK", "SIP/2.0/UDP biloxi.com:5060;branch=z9hG4bK");
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk", "SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk");
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk-999999-", "SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk-999999-");
   testParseAndWrite<Via>("SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk-524287-11-feedbeef-beefdead-uhoh", "SIP/2.0/UDP biloxi.com:5060;branch=Z9Hg4Bk-524287-11-feedbeef-beefdead-uhoh");

   // Performance tests
   // !bwc! Add command-line flag to enable/disable this.

   {
      resip::Data test("Raw header-field-value creation/deletion");
      cout << endl << test << endl;
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         HeaderFieldValue hfv(test.data(), test.size());
//         (hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   // Auth
   {
      resip::Data test("Auth creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Auth auth(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("Digest nonce=\"1305231689:fed475e952ee1d3ecaf60b17bce12218\",algorithm=MD5,realm=\"localhost\",qop=\"auth,auth-int\"");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Auth auth(hfv, Headers::UNKNOWN);
         auth.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Auth pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("Digest username=\"derek\",realm=\"localhost\",nonce=\"1305231689:fed475e952ee1d3ecaf60b17bce12218\",uri=\"sip:jason@localhost\",response=\"88f519f04c2a09c500af88ff7bccdf52\",cnonce=\"foo\",nc=0000005D,qop=auth-int,algorithm=MD5");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Auth auth(hfv, Headers::UNKNOWN);
         auth.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Auth pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   // CSeq
   {
      resip::Data test("CSeq creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         CSeqCategory cseq(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("9872643 INVITE");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         CSeqCategory cseq(hfv, Headers::UNKNOWN);
         cseq.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      CSeqCategory pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("1 INVITE");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         CSeqCategory cseq(hfv, Headers::UNKNOWN);
         cseq.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      CSeqCategory pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // CallId
   {
      resip::Data test("CallId creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         CallId callid(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("NOIUYCEOGoIUBaocuwyVCopiuVAbcs");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         CallId callid(hfv, Headers::UNKNOWN);
         callid.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      CallId pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // Date
   {
      resip::Data test("Date creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         DateCategory date(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("Thu, 21 Feb 2002 13:02:03 GMT");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         DateCategory date(hfv, Headers::UNKNOWN);
         date.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      DateCategory pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // Expires
   {
      resip::Data test("Expires creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         ExpiresCategory pc(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("3600");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         ExpiresCategory pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      ExpiresCategory pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // Mime
   {
      resip::Data test("Mime creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Mime pc(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }
   
   {
      resip::Data test("application/rlmi+xml");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Mime pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Mime pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }
   
   {
      resip::Data test("application/sdp");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Mime pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Mime pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }
   
   
   // NameAddr
   {
      resip::Data test("NameAddr creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("<sip:rls.example.com>");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("<sip:rls.example.com;lr>");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("<sipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsipsip:rls.example.com;lr>");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data strangeUser("работающий");
      strangeUser=strangeUser.charEncoded();
      resip::Data test("<sip:" + strangeUser+ "@example.com;lr>");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test(" \"Derek\" <sip:derek@localhost>;tag=16d1246e");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("<sip:line1@192.0.2.2;transport=tcp>; reg-id=1;+sip.instance=\"<urn:uuid:00000000-0000-1000-8000-000A95A0E128>\"");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("<sip:line1@192.0.2.2:5060;transport=tcp>; reg-id=1;+sip.instance=\"<urn:uuid:00000000-0000-1000-8000-000A95A0E128>\"");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         NameAddr pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      NameAddr pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // RequestLine
   {
      resip::Data test("RequestLine creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         RequestLine pc(hfv);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("INVITE sip:bob@example.com SIP/2.0");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         RequestLine pc(hfv);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      RequestLine pc(hfv);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("UNKNOWN sip:bob@example.com SIP/2.0");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         RequestLine pc(hfv);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      RequestLine pc(hfv);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // StatusLine
   {
      resip::Data test("StatusLine creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         StatusLine pc(hfv);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("SIP/2.0 200 OK");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         StatusLine pc(hfv);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      StatusLine pc(hfv);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("SIP/2.0 200 OKokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokokok");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         StatusLine pc(hfv);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      StatusLine pc(hfv);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("NOTSIP/2.0 200 OK");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         StatusLine pc(hfv);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      StatusLine pc(hfv);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   // Via
   {
      resip::Data test("Via creation/deletion");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Via pc(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("SIP/2.0/TCP 127.0.0.1:5060;branch=z9hG4bK-524287-1---307cd5596615cb2e;rport");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Via pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Via pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }


   {
      resip::Data test("");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Token pc(hfv, Headers::UNKNOWN);
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      resip::Data test("");
      cout << endl << test << endl;
      HeaderFieldValue hfv(test.data(), test.size());
      UInt64 now(Timer::getTimeMicroSec());
      for(int i=0; i<1000; ++i)
      {
         Token pc(hfv, Headers::UNKNOWN);
         pc.checkParsed();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
      cout << "encodes:" << endl;
      Token pc(hfv, Headers::UNKNOWN);
      pc.checkParsed();
      Data buffer;
      oDataStream str(buffer);
      now=Timer::getTimeMicroSec();
      for(int i=0; i<1000; ++i)
      {
         pc.encode(str);
         str.flush();
         str.reset();
      }
      cout << Timer::getTimeMicroSec() - now << " microseconds" << endl;
   }

   {
      TR _tr( "Test TokenOrQuotedStringCategory (token + parameters parse test)");

      Data tokenString = "uui-0123456789;purpose=\"abcd\"";
      ParseBuffer pb(tokenString.data(), tokenString.size());
      TokenOrQuotedStringCategory tok;
      tok.parse(pb);
      assert(tok.value() == "uui-0123456789");
      assert(tok.quotedValue() == "uui-0123456789");
      assert(tok.param(p_purpose) == "abcd");
   }

   {
      TR _tr( "Test TokenOrQuotedStringCategory (quoted string + parameters parse test)");

      Data quotedString = "\"the quick silver fox jumped over the lazy brown dog\";encoding=hex";
      ParseBuffer pb(quotedString.data(), quotedString.size());
      TokenOrQuotedStringCategory tok;
      tok.parse(pb);
      assert(tok.value() == "the quick silver fox jumped over the lazy brown dog");
      assert(tok.quotedValue() == "\"the quick silver fox jumped over the lazy brown dog\"");
      assert(tok.param(p_encoding) == Symbols::Hex);
   }

   assert(!failed);
   resipCerr << "\nTEST OK" << endl;

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
// vim: softtabstop=3 shiftwidth=3 expandtab
/* End: */
