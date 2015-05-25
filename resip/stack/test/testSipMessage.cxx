#include <iostream>
#include <memory>

#include "rutil/DataStream.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/test/TestSupport.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/UnknownHeaderType.hxx"
#include "resip/stack/UnknownParameterType.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Inserter.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define CRLF "\r\n"
#define RESIP_COOKIE "-524287-"

//vis -o to make binary bodies text

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   initNetwork();

   {
      // This test excercises a now fixed use-after-free bug when adding multi-headers to a list that has been copied, and then
      // one of the headers is accessed (but not necessarily parsed).  
      Data txt("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
         "Record-Route: <sip:rruser@rrdomain;lr>\r\n"
         "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
         "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
         "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
         "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
         "Call-ID: 6c64b42fce01b007\r\n"
         "CSeq: 2 INVITE\r\n"
         "Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
         "Contact: <sip:192.168.2.15:5100>\r\n"
         "Content-Length: 0\r\n"
         "\r\n");

      auto_ptr<SipMessage> msg(SipMessage::make(txt, true /* isExternal */));

      SipMessage response;
      Helper::makeResponse(response, *msg, 200);

      // Trigger a parse of Record-Route
      NameAddr rr("sip:test@rr.com");
      if(!(response.header(h_RecordRoutes).front() == rr))
      {
         //
      }
      // Now push a new Record-Route
      response.header(h_RecordRoutes).push_front(rr);
      assert(response.header(h_RecordRoutes).back().uri().user() == "rruser");

      //InfoLog(<< response);
   }

   static ExtensionParameter p_tag_ext("tag");
   {
      Data txt(
            "SIP/2.0 401 Unauthorized\r\n"
            "To: <sip:6309790007@ahenc1.ascc.lucent.com>;tag=43822895-1132606320408559\r\n"
            "From: \"Kit LDAP\"<sip:6309790007@ahenc1.ascc.lucent.com>;tag=64505823\r\n"
            "Call-ID: 7574556ad424b15c@aW5zcDc1MDAudW5pY29uLWludGwuY29t\r\n"
            "CSeq: 1 PUBLISH\r\n"
            "Via: SIP/2.0/UDP 67.184.22.204:33001;received=67.184.22.204;branch=z9hG4bK" RESIP_COOKIE "1---861ee62db418b378\r\n"
            "Server: Lucent SIPTRANS 1.2\r\n"
            "WWW-Authenticate: Digest realm=\"aP3nFt10ziWg41Su4s8\", \r\n"
            "   nonce=\"d6cb083cced5583c140c5f99eb81feda\", algorithm=MD5, qop=\"auth\", \r\n"
            "   opaque=\"f2a62109ee6526c8760e4e7497861aac\"\r\n"
            "Content-Length: 0\r\n"
            "\r\n"
            );

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      assert(msg.get());   
      resipCerr << Inserter(msg->header(h_WWWAuthenticates)) << endl;

      resipCerr << "[" << msg->header(h_WWWAuthenticates).back().param(p_opaque) << "]" << endl;

      assert(msg->header(h_WWWAuthenticates).back().param(p_opaque) == "f2a62109ee6526c8760e4e7497861aac");
   }

   {
      Data txt(
         "To: sip:fluffy@h1.cs.sipit.net\r\n"
         "From: tofu <sip:tofu@ua.ntt.sipit.net>;tag=5494179792598219348\r\n"
         "CSeq: 1 SUBSCRIBE\r\n"
         "Call-ID: 1129541551360711705\r\n"
         "Contact: sip:tofu@ua.ntt.sipit.net:5060\r\n"
         "Event: presence\r\n"
         "Content-Length: 0\r\n"
         "Expires: 3600\r\n"
         "User-Agent: NTT SecureSession User-Agent\r\n"
         "\r\n"
         "--Dfk2rISgfWIirrOJ\r\n"
         "Content-Type: application/pkcs7-signature; name=\"smime.p7s\"\r\n"
         "Content-Transfer-Encoding: binary\r\n"
         "Content-Disposition: attachment; filename=\"smime.p7s\"; handling=required\r\n"
         "0\202\004\201\327\006	*\206H\206\367\001\007\002\201\240\202\004\201\3100\202\004\201\304\002\001\0011\0130	\006\005+\003\002\032\005\000\201\260\013\006	\201\252\206\201\310\206\367\001\007\001\201\240\202\003\201\376\201\260\202\003\201\372\201\260\202\002\201\343\201\240\003\002\001\002\002\007\201\322\000\002\000\201\327\003\201\313\201\260\006	\201\252\206\201\310\206\367\001\001\005\005\000\201\260\201\360\201\261\013\201\260	\006\003\201\325\004\006\023\002\201\325\201\323\201\261\023\201\260\021\006\003\201\325\004\010\023r\201\303\201\341\201\354\201\351\201\346\201\357\201\362\201\356\201\351\201\341\201\261\021\201\260\006\003U\004\007\023\010San Jose1\201\260\014\006\003\201\325\004\023\005\201\363\201\351\201\360\201\351\201\364\201\261\201\251\201\260\201\247\006\003\201\325\004\013\023\201\240\201\323\201\351\201\360\201\351\201\364\201\240\201\324\201\345\201\363\201\364\201\240\201\303\201\345\201\362\201\364\201\351\201\346\201\351\201\343\201\341\201\364\201\345\201\240\201\301\201\365\201\364\201\350\201\357\201\362\201\351\201\364\201\371\201\260\036\027\201\260\201\264\201\260\201\270\201\262\201\265\201\260\201\262\201\261\201\270\201\264\201\270\201\332\027\201\260\201\267\201\260\201\270\201\262\201\265\201\260\201\262\201\261\201\270\201\264\201\270\201\332\201\260\201\345\201\261\013\201\260	\006\003\201\325\004\006\023\002\201\325\201\323\201\261\023\201\260\021\006\003\201\325\004\010\023\201\303\201\341\201\354\201\351\201\346\201\357\201\362\201\356\201\351\201\341\201\261\021\201\260\006\003U\004\007\023\010San Jose1\201\260\014\006\003\201\325\004\005\201\363\201\351\201\360\201\351\201\364\201\261\036\201\260\034\006\003\201\325\004\003\024\025\201\364\201\357\201\346\201\365\201\300\201\365\201\341\201\256\201\356\201\364\201\364\201\256\201\363\201\351\201\360\201\351\201\364\201\256\201\356\201\345\201\364\201\260\201\237\201\260\006	\201\252\206\201\310\206\367\001\001\001\005\000\003\201\215\000\201\260\201\211\002\201\201\000\201\331\201\322\201\267\201\345\201\263\201\337\201\201\365\201\246\201\317\201\337\201\314\201\343\201\350\201\370\201\332\201\372\201\366\201\356\227\201\260\010\201\304\201\326\005\201\253\014\201\316\201\335\201\362\201\253\201\336\036\201\326\201\247\201\365\201\321\201\342\201\312\201\336\201\310\030\201\310\034\201\325\201\274\201\255\201\336\201\372\201\306\201\277\201\303\201\364\201\343\201\355\201\240\201\323\201\374\201\347\201\240\004\201\363\205\316\230\205\320\205\261\205\255\211\205\205\261\205\276\205\333\205\277\205\350\205\274\205\361\021\201\260\201\255\201\265\201\276\201\373\203\201\267\201\356\201\355\201\256\201\337\201\262\201\244\201\305\033\023\000\201\266\206\021\201\303\201\252\201\265\201\364\001\201\246\201\251\030\201\372\235\201\325\201\273\037\201\251\201\332\201\354\201\331\201\371\201\263\201\261\201\347\236\201\322\201\376\201\274\201\332\201\377\002\003\001\000\001\201\243\202\001\201\247\201\260\202\001\201\243\201\260\201\332\006\003\201\325\035\021\004\201\323\201\260\201\321\206\031\201\363\201\351\201\360\201\272\201\364\201\357\201\346\201\365\201\300\201\365\201\341\201\256\201\356\201\364\201\364\201\256\201\363\201\351\201\360\201\351\201\364\201\256\201\356\201\345\201\364\206\030\201\351\201\355\201\272\201\364\201\357\201\346\201\365\201\300\201\365\201\341\201\256\201\356\201\364\201\364\201\256\201\363\201\351\201\360\201\351\201\364\201\256\201\356\201\345\201\364\206\032\201\360\201\362\201\345\201\363\201\272\201\364\201\357\201\346\201\365\201\300\201\365\201\341\201\256\201\356\201\364\201\364\201\256\201\363\201\351\201\360\201\351\201\364\201\256\201\356\201\345\201\364\201\260	\006\003\201\325\035\023\004\002\201\260\000\201\260\035\006\003\201\325\035\004\026\004\024	\026\201\307\201\306l\201\242\234\013\201\261\034\237\201\247\201\262\035\201\254B\201\357\201\324\201\2660\201\232\006\003U\035#\004\201\2220\201\217\200\024kF\027\024\201\352\224v%\200Tn\023T\201\332\201\241\201\343T\024\201\241\201\266\201\241t\201\244r0p1\0130	\006\003U\004\006\023\002US1\0230\021\006\003U\004\010\023California1\0210\006\003U\004\007\023\010San Jose1\201\260\014\006\003\201\325\004\023\005\201\363\201\351\201\360\201\351\201\364\201\261\201\251\201\260\201\247\006\003\201\325\004\013\023\201\240\201\323\201\351\201\360\201\351\201\364\201\240\201\324\201\345\201\363\201\364\201\240\201\303\201\345\201\362\201\364\201\351\201\346\201\351\201\343\201\341\201\364\201\345\201\240\201\301\201\365\201\364\201\350\201\357\201\362\201\351\201\364\201\371\202\001\000\201\260\006	\201\252\206\201\310\206\367\001\001\005\005\000\003\201\201\000\201\241\201\311\201\336\226\201\321\201\365\201\256\201\256\005\201\245\215\310\215\320\215\260\024\201\351\201\301\201\377\201\270\201\337\201\260\201\306\203\201\241\201\307\203\355\032\201\246\003\201\334\201\264\010\201\321\000\201\261\201\255\201\261\201\366\201\304\201\346\201\270\201\313\226\210\201\363\014\201\256\201\277\201\302\201\240\201\361\201\366\201\352\201\344\201\255\201\327\201\263\236\201\355\201\272\010\236\201\277\201\245\201\267\201\316\201\300\201\311\201\357\201\336\201\277\201\360\201\366\201\302\201\262\201\373\201\261\201\351\201\345\201\243\201\330\201\325\201\271\201\277\201\264\234\201\363\007\021\201\247\201\314\201\265\201\243\201\242\201\253\201\353\201\266\201\313\201\341\201\341\201\340\201\316\232\205\244\205\324\205\267\205\363\205\351\205\257\205\373\205\267\010\201\362\201\240\201\273\201\324\201\265\211\264\030\201\372\201\356\201\326\201\323\201\261\202\001\201\241\201\260\202\001\035\002\001\001\201\260\201\373\201\260\201\360\201\261\013\201\260	\006\003\201\325\004\006\023\002\201\325\201\323\201\261\023\201\260\021\006\003\201\325\004\010\023\201\303\201\341\201\354\201\351\201\346\201\357\201\362\201\356\201\351\201\341\201\261\021\201\260\006\003U\004\007\023\010San Jose1\201\260\014\006\003\201\325\004\023\005\201\363\201\351\201\360\201\351\201\364\201\261\201\251\201\260\201\247\006\003\201\325\004\013\023\201\240\201\323\201\351\201\360\201\351\201\364\201\240\201\324\201\345\201\363\201\364\201\240\201\303\201\345\201\362\201\364\201\351\201\346\201\351\201\343\201\341\201\364\201\345\201\240\201\301\201\365\201\364\201\350\201\357\201\362\201\351\201\364\201\371\002\007\201\322\000\002\000\201\327\003\201\313\201\260	\006\005\201\253\003\002\032\005\000\201\260\006	\201\252\206\201\310\206\367\001\001\001\005\000\004\201\200\201\246\201\255\202\350\202\242\202\261\207\227\033\027\201\375	\201\332\201\267\201\246\201\375\031\201\326\201\255\001\201\271\201\333\201\306\201\351\201\367\201\344\031\201\356\233\201\353\031\003\201\336	\003\201\353\201\260\201\323\201\267\026\022\201\307\233\201\316\201\362\201\304\201\256\201\247\201\243\201\256\201\261\211\202\243\202\324\202\314\027\222\027\222\034\230\201\240\201\323\037\201\345\034\220\201\372\201\261\201\377\201\333\201\310\201\343\201\300\201\335\201\310	\236\201\274\201\315\201\301\201\352\202\201\261\201\324\201\347\222\203\333\203\316\203\336\203\360<\211\236\203\276[\203\301\003\010\201\373rR\236\201\322\027\201\242\201\374b\027d:\225(\030\225\217\032\202\025\201\373\226\201\255\201\255\201\304\201\346\201\353\201\262\201\362\201\311\201\323\201\347\201\346\201\327\201\311\201\351\201\362\201\362\201\317\201\312\201\255\201\255" );

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      // !ah! deleted resipCerr output that was messing up terminals 
   }

   {
      SipMessage empty;

      try
      {
         resipCerr << empty.brief() << endl;
         empty.header(h_CSeq);
         resipCerr << empty.brief() << endl;
         empty.header(h_Vias).push_back(Via());
         resipCerr << empty.brief() << endl;
      }
      catch (SipMessage::Exception& e)
      {}
   }
   
   {
      Data txt("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
               "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
               "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
               "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
               "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
               "Call-ID: 6c64b42fce01b007\r\n"
               "CSeq: 2 INVITE\r\n"
               "Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
               "Contact: <sip:192.168.2.15:5100>\r\n"
               "Content-Length: 0\r\n"
               "\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      msg->header(h_Routes).pop_back();
      resipCerr << *msg << endl;
   }
   
   {
      Data txt("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
               "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
               "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
               "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
               "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
               "Call-ID: 6c64b42fce01b007\r\n"
               "CSeq: 2 INVITE\r\n"
               "Record-Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
               "Contact: <sip:192.168.2.15:5100>\r\n"
               "Max-Forwards: 69\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 307\r\n"
               "\r\n"
               "v=0\r\n"
               "o=M2TUA 1589993278 1032390928 IN IP4 192.168.2.15\r\n"
               "s=-\r\n"
               "c=IN IP4 192.168.2.15\r\n"
               "t=0 0\r\n"
               "m=audio 9000 RTP/AVP 103 97 100 101 0 8 102\r\n"
               "a=rtpmap:103 ISAC/16000\r\n"
               "a=rtpmap:97 IPCMWB/16000\r\n"
               "a=rtpmap:100 EG711U/8000\r\n"
               "a=rtpmap:101 EG711A/8000\r\n"
               "a=rtpmap:0 PCMU/8000\r\n"
               "a=rtpmap:8 PCMA/8000\r\n"
               "a=rtpmap:102 iLBC/8000\r\n");
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());

      sdp->session().media().front();
      
      SdpContents osdp;
      osdp.session().version() = 0;
      osdp.session().name() = "-";
      osdp.session().origin() = SdpContents::Session::Origin(Data("M2TUA"), 
                                                             77,
                                                             99,
                                                             SdpContents::IP4,
                                                             "localhost");
      osdp.session().addTime(SdpContents::Session::Time(0,0));
      osdp.session().connection() = 
         SdpContents::Session::Connection(SdpContents::IP4, "localhost", 0); 
      SdpContents::Session::Medium medium(Symbols::audio, 0, 1, Symbols::RTP_AVP);

      SdpContents::Session::Codec codec1("iLBC", 102, 8000);
      medium.addCodec(codec1);

      SdpContents::Session::Codec codec2("PCMA", 8, 8000);
      medium.addCodec(codec2);

      SdpContents::Session::Codec codec3("PCMU", 0, 8000);
      medium.addCodec(codec3);

      SdpContents::Session::Codec codec4("EG711A", 101, 8000);
      medium.addCodec(codec4);

      SdpContents::Session::Codec codec5("EG711U", 100, 8000);
      medium.addCodec(codec5);

      SdpContents::Session::Codec codec6("IPCMWB", 97, 16000);
      medium.addCodec(codec6);

      SdpContents::Session::Codec codec7("ISAC", 103, 16000);
      medium.addCodec(codec7);

      osdp.session().addMedium(medium);
      SdpContents::Session::Medium& myMedium = osdp.session().media().front();
      
      SdpContents csdp;
      csdp = *sdp;
      SdpContents::Session::Medium& my1Medium = csdp.session().media().front();
      resipCerr << myMedium.protocol() << " : " << my1Medium.protocol() << endl;

      SdpContents c2sdp;
      c2sdp = *sdp;
    

      SdpContents::Session::Medium& myMedium2 = c2sdp.session().media().front();
      resipCerr << myMedium2.protocol() << endl;
   }

   {
      Data txt1 = "SIP/2.0 407 Proxy Authentication Required\r\n"
         "To: <sip:jason_AT_meet2talk.com@beta.meet2talk.com>\r\n"
         "From: <sip:jason_AT_meet2talk.com@beta.meet2talk.com>;tag=113cba09\r\n"
         "Via: SIP/2.0/64.124.66.32:9091;branch=z9hG4bK-c87542-5b42cb698e8c6827790212ac5bdade1a-1-PA32768-c87542-;rport;received=64.124.66.32\r\n"
         "Via: SIP/2.0/UDP 192.168.1.102:5100;branch=z9hG4bK-c87542-175255966-1--c87542-;rport\r\n"
         "Call-ID: d8023c1dc2559a21\r\n"
         "CSeq: 1 REGISTER\r\n"
         "Contact: <sip:64.124.66.32:5060>\r\n"
         "Content-Length: 0\r\n\r\n";

      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));

      try
      {
         resipCerr << "transport=" << message1->header(h_Vias).front().transport() << endl;
         assert(false);
      }
      catch (ParseException& e)
      {
      }
   }

   {
      resipCerr << "test CRLFs before the start line" << endl;
      Data txt1 = "\r\n\r\n\r\nSIP/2.0 407 Proxy Authentication Required\r\n"
         "To: <sip:jason_AT_meet2talk.com@beta.meet2talk.com>\r\n"
         "From: <sip:jason_AT_meet2talk.com@beta.meet2talk.com>;tag=113cba09\r\n"
         "Via: SIP/2.0/UDP 64.124.66.32:9091;branch=z9hG4bK-c87542-5b42cb698e8c6827790212ac5bdade1a-1-PA32768-c87542-;rport;received=64.124.66.32\r\n"
         "Via: SIP/2.0/UDP 192.168.1.102:5100;branch=z9hG4bK-c87542-175255966-1--c87542-;rport\r\n"
         "Call-ID: d8023c1dc2559a21\r\n"
         "CSeq: 1 REGISTER\r\n"
         "Contact: <sip:64.124.66.32:5060>\r\n"
         "Content-Length: 0\r\n\r\n";

      try
      {
         auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));
         if (message1->header(h_StatusLine).statusCode() == 407){;}
      }
      catch (BaseException& e)
      {
         assert(false);
      }
   }

   {
      resipCerr << "test complex content copy" << endl;

      Data txt = ("MESSAGE sip:fluffy@212.157.205.40 SIP/2.0\r\n"
                  "Via: SIP/2.0/TCP 212.157.205.198:5060;branch=z9hG4bK2367411811584019109\r\n"
                  "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-107338443-1--c87542-;stid=489573115\r\n"
                  "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1--c87542-;received=192.168.2.220\r\n"
                  "To: sip:fluffy@212.157.205.40\r\n"
                  "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
                  "Max-Forwards: 70\r\n"
                  "CSeq: 1 NOTIFY\r\n"
                  "Call-ID: 28067261571992032320\r\n"
                  "Contact: sip:ntt2@212.157.205.198:5060\r\n"
                  "Expires: 47\r\n"
                  "Content-Length: 1929\r\n"
                  "Content-Type: multipart/signed;\r\n"
                  " protocol=\"application/pkcs7-signature\";\r\n"
                  " micalg=sha1; boundary=\"----YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\"\r\n"
                  "\r\n"
                  "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
                  "Content-Type: multipart/mixed;boundary=\"----lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\"\r\n"
                  "Content-Length: 870\r\n"
                  "Content-Disposition: attachment;handling=required\r\n"
                  "\r\n"
                  "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
                  "Content-Type: application/sipfrag\r\n"
                  "Content-Length: 320\r\n"
                  "\r\n"
                  "To: sip:fluffy@212.157.205.40\r\n"
                  "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
                  "CSeq: 1 NOTIFY\r\n"
                  "Call-ID: 28067261571992032320\r\n"
                  "Contact: sip:ntt2@212.157.205.198:5060\r\n"
                  "Event: presence\r\n"
                  "Content-Length: 210\r\n"
                  "Content-Type: application/xpidf+xml\r\n"
                  "Subscription-State: active\r\n"
                  "User-Agent: XXX SecureSession User-Agent\r\n"
                  "\r\n"
                  "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
                  "Content-Type: application/xpidf+xml\r\n"
                  "Content-Length: 210\r\n"
                  "\r\n"
                  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                  "<presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\" entity=\"pres:someone@example.com\">\r\n"
                  "<tuple id=\"765\">\r\n"
                  "<status>\r\n"
                  "<basic>open</basic>\r\n"
                  "</status>\r\n"
                  "</tuple>\r\n"
                  "</presence>\r\n"
                  "\r\n"
                  "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY--\r\n"
                  "\r\n"
                  "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
                  "Content-Type: application/pkcs7-signature; name=\"smime.p7s\"\r\n"
                  "Content-Transfer-Encoding: base64\r\n"
                  "Content-Disposition: attachment; filename=\"smime.p7s\"; handling=required\r\n"
                  "\r\n"
                  "MIIBVgYJKoZIhvcNAQcCoIIBRzCCAUMCAQExCzAJBgUrDgMCGgUAMAsGCSqGSIb3\r\n"
                  "DQEHATGCASIwggEeAgEBMHwwcDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlm\r\n"
                  "b3JuaWExETAPBgNVBAcTCFNhbiBKb3NlMQ4wDAYDVQQKEwVzaXBpdDEpMCcGA1UE\r\n"
                  "CxMgU2lwaXQgVGVzdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkCCAIWABUCBgIVMAkG\r\n"
                  "BSsOAwIaBQAwDQYJKoZIhvcNAQEBBQAEgYAer8TPSMtA3ZqweGnXLUYKR51bp52N\r\n"
                  "oGBEqHZz7xR0Nhs6DsAOXiSFv19vTR//33u6Se3zpNNHP/zj7NRr+olimI2PeBNB\r\n"
                  "tczNdqexoN0pjRW7l7mHZ0e39pqZmI5bhFl+z9CJJu5xW0aSarw84CZxbh5RQaYr\r\n"
                  "zhSvTYdki20aiQ==\r\n"
                  "\r\n"
                  "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC--\r\n"
         );

      std::auto_ptr<SipMessage> msg(SipMessage::make(txt));
      
      SipMessage msg1;
      msg1.setContents(msg->getContents());

      CerrLog(<< Data::from(*msg1.getContents()));
      CerrLog(<< Data::from(*msg->getContents()));
   }

   {
      InfoLog(<< "Testing assignment");

      Data txt1 = ("SIP/2.0 401 Unauthorized\r\n"
                   "To: <sip:foobie@example.com>;tag=12345678\r\n"
                   "From: <sip:bar@example.com>;tag=83ec8345\r\n"
                   "Via: SIP/2.0/TLS 192.168.2.205:5061;branch=z9hG4bK-c87542-488593999-1--c87542-;rport\r\n"
                   "Call-ID: 3f0b546f89f28456\r\n"
                   "CSeq: 1 REGISTER\r\n"
                   "Expires: 3600\r\n"
                   "Max-Forwards: 70\r\n"
                   "Www-Authenticate: Basic realm=test\r\n"
                   "Allow-Events: presence\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));


      Data txt2 = ("NOTIFY sip:fluffy@212.157.205.40 SIP/2.0\r\n"
                   "Via: SIP/2.0/TCP 212.157.205.198:5060;branch=z9hG4bK2367411811584019109\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-107338443-1--c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1--c87542-;received=192.168.2.220\r\n"
                   "To: sip:fluffy@212.157.205.40\r\n"
                   "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
                   "Max-Forwards: 70\r\n"
                   "CSeq: 1 NOTIFY\r\n"
                   "Call-ID: 28067261571992032320\r\n"
                   "Contact: sip:ntt2@212.157.205.198:5060\r\n"
                   "Content-Length: 1929\r\n"
                   "Content-Type: multipart/signed;\r\n"
                   " protocol=\"application/pkcs7-signature\";\r\n"
                   " micalg=sha1; boundary=\"----YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\"\r\n"
                   "\r\n"
                   "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
                   "Content-Type: multipart/mixed;boundary=\"----lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\"\r\n"
                   "Content-Length: 870\r\n"
                   "Content-Disposition: attachment;handling=required\r\n"
                   "\r\n"
                   "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
                   "Content-Type: application/sipfrag\r\n"
                   "Content-Length: 320\r\n"
                   "\r\n"
                   "To: sip:fluffy@212.157.205.40\r\n"
                   "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
                   "CSeq: 1 NOTIFY\r\n"
                   "Call-ID: 28067261571992032320\r\n"
                   "Contact: sip:ntt2@212.157.205.198:5060\r\n"
                   "Event: presence\r\n"
                   "Content-Length: 210\r\n"
                   "Content-Type: application/xpidf+xml\r\n"
                   "Subscription-State: active\r\n"
                   "User-Agent: XXX SecureSession User-Agent\r\n"
                   "\r\n"
                   "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
                   "Content-Type: application/xpidf+xml\r\n"
                   "Content-Length: 210\r\n"
                   "\r\n"
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                   "<presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\" entity=\"pres:someone@example.com\">\r\n"
                   "<tuple id=\"765\">\r\n"
                   "<status>\r\n"
                   "<basic>open</basic>\r\n"
                   "</status>\r\n"
                   "</tuple>\r\n"
                   "</presence>\r\n"
                   "\r\n"
                   "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY--\r\n"
                   "\r\n"
                   "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
                   "Content-Type: application/pkcs7-signature; name=\"smime.p7s\"\r\n"
                   "Content-Transfer-Encoding: base64\r\n"
                   "Content-Disposition: attachment; filename=\"smime.p7s\"; handling=required\r\n"
                   "\r\n"
                   "MIIBVgYJKoZIhvcNAQcCoIIBRzCCAUMCAQExCzAJBgUrDgMCGgUAMAsGCSqGSIb3\r\n"
                   "DQEHATGCASIwggEeAgEBMHwwcDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlm\r\n"
                   "b3JuaWExETAPBgNVBAcTCFNhbiBKb3NlMQ4wDAYDVQQKEwVzaXBpdDEpMCcGA1UE\r\n"
                   "CxMgU2lwaXQgVGVzdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkCCAIWABUCBgIVMAkG\r\n"
                   "BSsOAwIaBQAwDQYJKoZIhvcNAQEBBQAEgYAer8TPSMtA3ZqweGnXLUYKR51bp52N\r\n"
                   "oGBEqHZz7xR0Nhs6DsAOXiSFv19vTR//33u6Se3zpNNHP/zj7NRr+olimI2PeBNB\r\n"
                   "tczNdqexoN0pjRW7l7mHZ0e39pqZmI5bhFl+z9CJJu5xW0aSarw84CZxbh5RQaYr\r\n"
                   "zhSvTYdki20aiQ==\r\n"
                   "\r\n"
                   "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC--\r\n"
         );

      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      SipMessage message3;
      UnknownHeaderType h_Foo("foo");
      UnknownHeaderType h_Bar("bar");

      message3.header(h_Foo);
      message3.header(h_Bar).push_back(StringCategory("bar1"));
      message3.header(h_Bar).push_back(StringCategory("bar2"));

      //message3.header(h_Vias);
      message3.header(h_To);
      
      message3.header(h_RequestLine) = RequestLine(INVITE);
      message3.header(h_RequestLine).uri() = Uri("sip:bob@biloxi.com");
      message3.header(h_To) = NameAddr("sip:bob@biloxi.com");
      message3.header(h_From) = NameAddr("Alice <sip:alice@atlanta.com>;tag=1928301774");
      message3.header(h_CallId).value() = "314159";
      message3.header(h_CSeq).sequence() = 14;
      message3.header(h_CSeq).method() = INVITE;

      PlainContents pc("here is some plain ol' contents");
      message3.setContents(&pc);

      message3 = *message2;

      // cause some parsing
      assert(!message2->header(h_To).exists(p_tag));
      message2->getContents();

      // cause some parsing
      assert(message1->header(h_Vias).front().param(p_branch).clientData().empty());

      assert(message1.get());
      *message2 = *message1;

      // cause some parsing
      message3.getContents();
      message3 = *message2;

      CerrLog(<< Data::from(message3));
      CerrLog(<< Data::from(*message1));

      assert(message1.get());
      assert(message3.isRequest() == message1->isRequest());
      assert(message3.isResponse() == message1->isResponse());
      
      assert(Data::from(message3) == Data::from(*message1));
   }
   
// !dlb! we seem to have relaxed the quoting requirement in general
// we accept unquoted when the grammar says quoted, but always emit quoted
#if 0
   {
      char* txt = ("SIP/2.0 401 Unauthorized\r\n"
                   "To: <sip:foobie@example.com>;tag=12345678\r\n"
                   "From: <sip:bar@example.com>;tag=83ec8345\r\n"
                   "Via: SIP/2.0/TLS 192.168.2.205:5061;branch=z9hG4bK-c87542-488593999-1--c87542-;rport\r\n"
                   "Call-ID: 3f0b546f89f28456\r\n"
                   "CSeq: 1 REGISTER\r\n"
                   "Expires: 3600\r\n"
                   "Max-Forwards: 70\r\n"
                   "Www-Authenticate: Basic realm=test\r\n"
                   "Allow-Events: presence\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");
      try
      {
         auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
         assert(message->exists(h_WWWAuthenticates));
         assert(message->header(h_WWWAuthenticates).size() == 1);
         assert(message->header(h_WWWAuthenticates).front().scheme() == "Basic");
         assert(0);
      }
      catch (ParseException& e)
      {
         InfoLog (<< "Rejected unquoted realm");
      }
   }
#endif

   {
      Data txt(
         "NOTIFY sip:fluffy@212.157.205.40 SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 212.157.205.198:5060;branch=z9hG4bK2367411811584019109\r\n"
         "To: sip:fluffy@212.157.205.40\r\n"
         "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
         "Max-Forwards: 70\r\n"
         "CSeq: 1 NOTIFY\r\n"
         "Call-ID: 28067261571992032320\r\n"
         "Contact: sip:ntt2@212.157.205.198:5060\r\n"
         "Content-Length: 1929\r\n"
         "Content-Type: multipart/signed;\r\n"
         " protocol=\"application/pkcs7-signature\";\r\n"
         " micalg=sha1; boundary=\"----YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\"\r\n"
         "\r\n"
         "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
         "Content-Type: multipart/mixed;boundary=\"----lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\"\r\n"
         "Content-Length: 870\r\n"
         "Content-Disposition: attachment;handling=required\r\n"
         "\r\n"
         "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
         "Content-Type: application/sipfrag\r\n"
         "Content-Length: 320\r\n"
         "\r\n"
         "To: sip:fluffy@212.157.205.40\r\n"
         "From: sip:ntt2@h1.ntt2.sipit.net;tag=727823805122397238\r\n"
         "CSeq: 1 NOTIFY\r\n"
         "Call-ID: 28067261571992032320\r\n"
         "Contact: sip:ntt2@212.157.205.198:5060\r\n"
         "Event: presence\r\n"
         "Content-Length: 210\r\n"
         "Content-Type: application/xpidf+xml\r\n"
         "Subscription-State: active\r\n"
         "User-Agent: XXX SecureSession User-Agent\r\n"
         "\r\n"
         "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY\r\n"
         "Content-Type: application/xpidf+xml\r\n"
         "Content-Length: 210\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\" entity=\"pres:someone@example.com\">\r\n"
         "<tuple id=\"765\">\r\n"
         "<status>\r\n"
         "<basic>open</basic>\r\n"
         "</status>\r\n"
         "</tuple>\r\n"
         "</presence>\r\n"
         "\r\n"
         "------lRIGMC8E2Px6I2IODfk2rISgfWIirrOJwS3tY52HuxDP3pdTiFjsghJJWhvyRCEY--\r\n"
         "\r\n"
         "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC\r\n"
         "Content-Type: application/pkcs7-signature; name=\"smime.p7s\"\r\n"
         "Content-Transfer-Encoding: base64\r\n"
         "Content-Disposition: attachment; filename=\"smime.p7s\"; handling=required\r\n"
         "\r\n"
         "MIIBVgYJKoZIhvcNAQcCoIIBRzCCAUMCAQExCzAJBgUrDgMCGgUAMAsGCSqGSIb3\r\n"
         "DQEHATGCASIwggEeAgEBMHwwcDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlm\r\n"
         "b3JuaWExETAPBgNVBAcTCFNhbiBKb3NlMQ4wDAYDVQQKEwVzaXBpdDEpMCcGA1UE\r\n"
         "CxMgU2lwaXQgVGVzdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkCCAIWABUCBgIVMAkG\r\n"
         "BSsOAwIaBQAwDQYJKoZIhvcNAQEBBQAEgYAer8TPSMtA3ZqweGnXLUYKR51bp52N\r\n"
         "oGBEqHZz7xR0Nhs6DsAOXiSFv19vTR//33u6Se3zpNNHP/zj7NRr+olimI2PeBNB\r\n"
         "tczNdqexoN0pjRW7l7mHZ0e39pqZmI5bhFl+z9CJJu5xW0aSarw84CZxbh5RQaYr\r\n"
         "zhSvTYdki20aiQ==\r\n"
         "\r\n"
         "------YvhIjyjTU8lfNqGe8Fyvxcmb4mleF6quxsMFpT2hOhrDfS3LLs1MyYBdLNgBLsSC--\r\n"
         );
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      InfoLog (<< "msg->header(h_ContentLength).value() == " << msg->header(h_ContentLength).value());
      assert( msg->header(h_ContentLength).value() == 1929 );
   }
    {
       Data txt( 
          "MESSAGE sip:fluffy@h1.cisco1.sipit.net:5060;transport=UDP SIP/2.0\r\n"
          "To: <sip:fluffy@h1.cisco1.sipit.net:5060>\r\n"
          "From: <sip:user@localhost:5080>;tag=20f94fd6\r\n"
          "Via: SIP/2.0/UDP 212.157.205.40:5080;branch=z9hG4bK-c87542-1005764096-2--c87542-;rport=5080;received=212.157.205.40\r\n"
          "Call-ID: 16f7f8fd368d8bcd\r\n"
          "CSeq: 1 MESSAGE\r\n"
          "Contact: <sip:user@212.157.205.40:5080>\r\n"
          "Max-Forwards: 70\r\n"
          "Content-Disposition: attachment;handling=required;filename=smime.p7\r\n"
          "Content-Type: application/pkcs7-mime;smime-type=enveloped-data;name=smime.p7m\r\n"
          "User-Agent: SIPimp.org/0.2.3 (curses)\r\n"
          "Content-Length: 4\r\n"
          "\r\n"
          "1234" );

       auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));  

       assert( msg->exists(h_ContentDisposition)  );
       msg->header(h_ContentDisposition);
       assert( msg->header(h_ContentLength).value() == 4 );
    }
   {
      // exercise header remove
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "Allow-Events: foo\r\n"
                   "Allow-Events: bar\r\n"
                   "Allow-Events: baz\r\n"
                   "Allow-Events: quux\r\n"
                   "Unsupported: \r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-107338443-1--c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1--c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Proxy-Authorization: Digest username=\"Betty\", realm = \"fresno.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "Unknown: foobie\r\n"
                   "Unknown: biefoo\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      // note message has not been parsed

      message->remove(h_AllowEvents);
      message->remove(h_Unsupporteds);
      message->remove(h_To);
      message->remove(h_From);
      message->remove(h_Vias);
      message->remove(h_CallId);
      message->remove(h_CSeq);
      message->remove(h_ProxyAuthorizations);
      message->remove(h_RecordRoutes);
      message->remove(h_Contacts);
      message->remove(h_MaxForwards);
      message->remove(h_ContentLength);
      message->remove(UnknownHeaderType("Unknown"));

      Data enc;
      {
         DataStream str(enc);
         str << *message;
      }

      InfoLog (<< enc);
      assert(enc == ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n"));
   }

   {
      // exercise header remove
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "Allow-Events: foo\r\n"
                   "Allow-Events: bar\r\n"
                   "Allow-Events: baz\r\n"
                   "Allow-Events: quux\r\n"
                   "Unsupported: \r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-107338443-1--c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1--c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Proxy-Authorization: Digest username=\"Betty\", realm = \"fresno.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "Unknown: foobie\r\n"
                   "Unknown: biefoo\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      message->header(h_AllowEvents).front().value();
      message->header(h_Unsupporteds).empty();
      message->header(h_To).uri().user();
      message->header(h_From);
      message->header(h_Vias).front();
      message->header(h_CallId).value();
      message->header(h_CSeq).sequence();
      message->header(h_ProxyAuthorizations).size();
      message->header(h_RecordRoutes).front().uri().host();
      message->header(h_Contacts).size();
      message->header(h_MaxForwards).value();
      message->header(h_ContentLength).value();
      message->header(UnknownHeaderType("Unknown")).size();

      // note message has been parsed
      message->remove(h_AllowEvents);
      message->remove(h_Unsupporteds);
      message->remove(h_To);
      message->remove(h_From);
      message->remove(h_Vias);
      message->remove(h_CallId);
      message->remove(h_CSeq);
      message->remove(h_ProxyAuthorizations);
      message->remove(h_RecordRoutes);
      message->remove(h_Contacts);
      message->remove(h_MaxForwards);
      message->remove(h_ContentLength);
      message->remove(UnknownHeaderType("Unknown"));
      
      // not present
      message->remove(h_Routes);

      Data enc;
      {
         DataStream str(enc);
         str << *message;
      }

      assert(enc == ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n"));
   }

   {
      // demonstrate comma encoding
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "Allow-Events: foo\r\n"
                   "Allow-Events: bar\r\n"
                   "Allow-Events: baz\r\n"
                   "Allow-Events: quux\r\n"
                   "Unsupported: \r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-107338443-1--c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1--c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Proxy-Authorization: Digest username=\"Betty\", realm = \"fresno.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      assert(message->exists(h_Unsupporteds));
      resipCerr << message->header(h_AllowEvents).size() << endl;
      resipCerr << message->header(h_Vias).size() << endl;
      resipCerr << message->header(h_ProxyAuthorizations).size() << endl;
      resipCerr << *message << endl;
   }

   {
      SipMessage msg;
      msg.header(h_CallId).value() = "8d057730fba2cd5e";

      resipCerr << msg.header(h_CallId) << endl;

      const SipMessage msg1(msg);
      resipCerr << msg1.header(h_CallId) << endl;
   }

   {
     const char * txt = ("SIP/2.0 489 Bad Event" CRLF
                   "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1--c87542-" CRLF
                   "CSeq: " CRLF
                   "Call-ID:  f354ce714fb8a95c" CRLF
                   "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                   "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                   CRLF);
     TestSupport::prettyPrint(txt,strlen(txt));

     auto_ptr<SipMessage> response(TestSupport::makeMessage(txt,true));
     try
     {
        response->header(h_CSeq).method();
        assert(false);
     }
     catch (ParseException& e)
     {
        resipCerr << e << endl;
     }
   }

   {
     const char * txt = ("SIP/2.0 489 Bad Event" CRLF
                   "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1--c87542-" CRLF
                   "CSeq: " CRLF
                   "Call-ID:  f354ce714fb8a95c" CRLF
                   "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                   "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                   CRLF);
     TestSupport::prettyPrint(txt,strlen(txt));

     auto_ptr<SipMessage> response(TestSupport::makeMessage(txt,true));
     try
     {
        const SipMessage& constSip(*response);
        assert(!constSip.header(h_CSeq).isWellFormed());
     }
     catch (...)
     {
        assert(false);
     }
   }

   {
     const char * txt = ("SIP/2.0 489 Bad Event" CRLF
                   "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK" RESIP_COOKIE "1---899769382" CRLF
                   "CSeq: 1 SUBSCRIBE" CRLF
                   "Allow-Events: " CRLF
                   "Call-ID:  f354ce714fb8a95c" CRLF
                   "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                   "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                   CRLF);
     TestSupport::prettyPrint(txt,strlen(txt));

     auto_ptr<SipMessage> response(TestSupport::makeMessage(txt,true));
     assert(response->exists(h_AllowEvents));
     assert(response->header(h_AllowEvents).size() == 0);

     resipCerr << response->brief() << endl;
     assert(Data::from(response->brief()) == "SipResp: 489 tid=899769382 cseq=1 SUBSCRIBE / 1 from(wire)");
     
     const char * txt2 = ("SIP/2.0 489 Bad Event" CRLF
                    "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK" RESIP_COOKIE "1---899769382" CRLF
                    "CSeq: 1 SUBSCRIBE" CRLF
                    "Call-ID:  f354ce714fb8a95c" CRLF
                    "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                    "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                    "Allow-Events:" CRLF
                    CRLF);
     TestSupport::prettyPrint(txt2,strlen(txt2));

     auto_ptr<SipMessage> r2(TestSupport::makeMessage(txt2,true));
     assert(r2->exists(h_AllowEvents));
     assert(r2->header(h_AllowEvents).size() == 0);

     const char * txt3 =("SIP/2.0 489 Bad Event" CRLF
                   "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK" RESIP_COOKIE "1---899769382" CRLF
                   "CSeq: 1 SUBSCRIBE" CRLF
                   "Call-ID:  f354ce714fb8a95c" CRLF
                   "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                   "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                   "Allow-Events: foo" CRLF
                   "Allow-Events: bar" CRLF
                   "Allow-Events: " CRLF
                   CRLF);

     auto_ptr<SipMessage> r3(TestSupport::makeMessage(txt3,false));
     assert(r3->exists(h_AllowEvents));
     assert(r3->header(h_AllowEvents).size() == 2);
     assert(r3->header(h_AllowEvents).front().value() == "foo");
     resipCerr << r3->brief() << endl;
     assert(Data::from(r3->brief()) == "SipResp: 489 tid=899769382 cseq=1 SUBSCRIBE / 1 from(tu)");

     const char * txt4 = ("SIP/2.0 489 Bad Event" CRLF
                    "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK" RESIP_COOKIE "1---899769382" CRLF
                    "CSeq: 1 SUBSCRIBE" CRLF
                    "Call-ID:  f354ce714fb8a95c" CRLF
                    "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                    "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                    "Allow-Events: foo,foobar" CRLF
                    "Allow-Events: bar,gak" CRLF
                    "Allow-Events: " CRLF
                    CRLF);

     auto_ptr<SipMessage> r4(TestSupport::makeMessage(txt4,true));
     assert(r4->exists(h_AllowEvents) );

     resipCerr << r4->header(h_AllowEvents).size() << endl;
     resipCerr << r4->header(h_AllowEvents).front().value() << endl;

     assert(r4->header(h_AllowEvents).size() == 4);
     assert(r4->header(h_AllowEvents).front().value() == "foo");


     const char * txt5 = ("SIP/2.0 489 Bad Event" CRLF
                    "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK" RESIP_COOKIE "1---899769382" CRLF
                    "CSeq: 1 SUBSCRIBE" CRLF
                    "Call-ID:  f354ce714fb8a95c" CRLF
                    "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
                    "To:  <sip:RjS@127.0.0.1:5060>" CRLF
                    "Allow-Events:      " CRLF
                    "Allow-Events:   	     " CRLF
                    "Allow-Events: " CRLF
                    CRLF);

     auto_ptr<SipMessage> r5(TestSupport::makeMessage(txt5,true));
     assert(r5->exists(h_AllowEvents) );
     assert(r5->header(h_AllowEvents).size() == 0);
   }

   {
      // Test just in time parsing with comparison: NameAddr;
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "1-client_data--11111;stid=489573115\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Accept: foo/bar\r\n"
                   "Content-Type: bar/foo\r\n"
                   "Content-Encoding: foo\r\n"
                   "Content-Disposition: bar\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_ContentDisposition) < message->header(h_ContentEncoding));
      assert(message->header(h_ContentType) < message->header(h_Accepts).front());
      assert(message->header(h_To) < message->header(h_From));

      resipCerr << message->brief() << endl;
      assert(Data::from(message->brief()) == "SipReq:  INVITE ext101@192.168.2.220:5064 tid=11111 cseq=1 INVITE contact=ext103@192.168.2.220:5068 / 1 from(tu)");
   }
   
   {
      resipCerr << "!Proxy-Authorization params" << endl;
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "1---563465;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK" RESIP_COOKIE "1---489573115;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\",realm=\"atlanta.com\",nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", response=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_ProxyAuthorizations).size() == 2);
      assert(message->header(h_ProxyAuthorizations).front().param(p_realm) == "atlanta.com");
      assert(message->header(h_ProxyAuthorizations).front().param(p_response) == "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");

      resipCerr << *message << endl;
   }

   {
      const char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK" RESIP_COOKIE "1---21312;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK" RESIP_COOKIE "1---489573115;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      resipCerr << *message << endl;

      assert(message->header(h_Vias).front().param(UnknownParameterType("stid")) == "489573115");
      message->header(h_Vias).front().param(p_received) = "received";
      assert(message->header(h_Vias).front().param(UnknownParameterType("stid")) == "489573115");
   }

   {
      const char* txt = ("SIP/2.0 200 OK""\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e""\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f""\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124""\r\n"
                   "CSeq: 1 INVITE""\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-3e5a380c-1ac5646-adf1-1""\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf""\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>""\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>""\r\n"
                   "Content-Length:0\r\n\r\n");
      
  
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      resipCerr << *message << endl;
      message->header(h_Vias).front();
      message->header(h_From);
      message->header(h_To);
      resipCerr << message->header(h_Vias).front().param(p_branch).getTransactionId() << endl;
      resipCerr << message->header(h_Vias).back().param(p_branch).getTransactionId() << endl;
   }

   {
      const char *txt = ("SIP/2.0 200 OK\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-3e5a380c-1ac5646-adf1-1\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>\r\n"
                   "Content-Length:0\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      assert(message.get());
      for (Vias::iterator i = message->header(h_Vias).begin();
           i != message->header(h_Vias).end(); i++)
      {
         i->param(p_branch).encode(resipCerr);
         resipCerr << endl;
      }
   }

   {
      const char* txt = ("SIP/2.0 200 OK""\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e""\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f""\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124""\r\n"
                   "CSeq: 1 INVITE""\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-3e5a380c-1ac5646-adf1-1""\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf""\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>""\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>""\r\n"
                   "Content-Length:0\r\n\r\n");
      
   
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      resipCerr << *message << endl;
      message->header(h_Vias).front();
      message->header(h_From);
      message->header(h_To);
      resipCerr << message->header(h_Vias).front() << endl;
      resipCerr << message->header(h_Vias).back() << endl;
      resipCerr << message->header(h_Vias).front().param(p_branch).getTransactionId() << endl;
      resipCerr << message->header(h_Vias).back().param(p_branch).getTransactionId() << endl;
   }

   {
      const char *txt = ("To: <sip:106@kelowna.gloo.net>"
                   "From: <sip:106@kelowna.gloo.net>;tag=18c7b33a-430c-429c-9f46-e5b509264519\r\n"
                   "Via: SIP/2.0/UDP 192.168.2.15:10276;received=192.168.2.15\r\n"
                   "Call-ID: cb15283c-6efb-452e-aef2-5e44e02e2440@192.168.2.15\r\n"
                   "CSeq: 2 REGISTER\r\n"
                   "Contact: <sip:192.168.2.15:10276>;xmethods=\"INVITE, MESSAGE, INFO, SUBSCRIBE, OPTIONS, BYE, CANCEL, NOTIFY, ACK\"\r\n"
                   "Expires: 0\r\n"
                   "User-Agent: Windows RTC/1.0\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      resipCerr << message->header(h_Contacts).front().param(UnknownParameterType("xmethods")) << endl;
      resipCerr << *message << endl;
   }

   {
      Data txt1("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
      
      Data txt2("INVITE sip:joe@biloxi.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt3("INVITE sip:bob@biloxi.com;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt4("INVITE sip:bob@biloxi.com;user=phone;lr SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt5("INVITE sip:bob@biloxi.com;user=phone;lr;maddr=192.168.1.1 SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt6("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt7("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;user=phone;jason=foo SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt8("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt9("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt10("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314158 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt11("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314158 INVITE\r\n"
                "Max-Forwards: 73\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
      
      
      auto_ptr<SipMessage> msg1(TestSupport::makeMessage(txt1));
      auto_ptr<SipMessage> msg2(TestSupport::makeMessage(txt2));
      auto_ptr<SipMessage> msg3(TestSupport::makeMessage(txt3));
      auto_ptr<SipMessage> msg4(TestSupport::makeMessage(txt4));
      auto_ptr<SipMessage> msg5(TestSupport::makeMessage(txt5));
      auto_ptr<SipMessage> msg6(TestSupport::makeMessage(txt6));
      auto_ptr<SipMessage> msg7(TestSupport::makeMessage(txt7));
      auto_ptr<SipMessage> msg8(TestSupport::makeMessage(txt8));
      auto_ptr<SipMessage> msg9(TestSupport::makeMessage(txt9));
      auto_ptr<SipMessage> msg10(TestSupport::makeMessage(txt10));
      auto_ptr<SipMessage> msg11(TestSupport::makeMessage(txt11));

      assert(msg1->getTransactionId() == msg1->getTransactionId());
      resipCerr << "msg2=" << msg2->getTransactionId() << endl;
      resipCerr << "msg3=" << msg3->getTransactionId() << endl;
      assert(msg2->getTransactionId() != msg3->getTransactionId());
      assert(msg3->getTransactionId() == msg4->getTransactionId());
      assert(msg4->getTransactionId() != msg5->getTransactionId());
      assert(msg4->getTransactionId() != msg6->getTransactionId());
      
      resipCerr << "msg5=" << msg5->getTransactionId() << endl;
      resipCerr << "msg6=" << msg6->getTransactionId() << endl;
      assert(msg5->header(h_RequestLine).uri() == msg6->header(h_RequestLine).uri());
      assert(msg5->header(h_RequestLine).uri().commutativeParameterHash() == msg6->header(h_RequestLine).uri().commutativeParameterHash());

      assert(msg5->getTransactionId() == msg6->getTransactionId());
      assert(msg7->getTransactionId() == msg8->getTransactionId());
      assert(msg6->getTransactionId() != msg8->getTransactionId());
      assert(msg8->getTransactionId() != msg9->getTransactionId());
      assert(msg9->getTransactionId() != msg10->getTransactionId());
      assert(msg10->getTransactionId() == msg11->getTransactionId());
   }
   
   {
      SipMessage inv;

      UnknownHeaderType h_Foo("foo");
      UnknownHeaderType h_Bar("bar");

      inv.header(h_Foo);
      inv.header(h_Bar).push_back(StringCategory("bar1"));
      inv.header(h_Bar).push_back(StringCategory("bar2"));

      inv.header(h_To);
      
      inv.header(h_RequestLine) = RequestLine(INVITE);
      inv.header(h_RequestLine).uri() = Uri("sip:bob@biloxi.com");
      inv.header(h_To) = NameAddr("sip:bob@biloxi.com");
      inv.header(h_From) = NameAddr("Alice <sip:alice@atlanta.com>;tag=1928301774");
      inv.header(h_CallId).value() = "314159";
      inv.header(h_CSeq).sequence() = 14;
      inv.header(h_CSeq).method() = INVITE;

      PlainContents pc("here is some plain ol' contents");
      inv.setContents(&pc);

      resipCerr << inv.header(h_ContentType).type() << endl;
      assert(inv.header(h_ContentType).type() == "text");
      assert(inv.header(h_ContentType).subType() == "plain");

      assert(!inv.exists(h_ContentLength));

      assert(inv.getContents());
      assert(dynamic_cast<PlainContents*>(inv.getContents()));
      assert(dynamic_cast<PlainContents*>(inv.getContents())->text() == "here is some plain ol' contents");

      const Data d(Data::from(inv));
      
      resipCerr << "!! " << d;
      assert(d == ("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                   "To: <sip:bob@biloxi.com>\r\n"
                   "From: \"Alice\"<sip:alice@atlanta.com>;tag=1928301774\r\n"
                   "Call-ID: 314159\r\n"
                   "CSeq: 14 INVITE\r\n"
                   "Content-Type: text/plain\r\n"
                   "bar: bar1\r\n"
                   "bar: bar2\r\n"
                   "Content-Length: 31\r\n"
                   "\r\n"
                   "here is some plain ol' contents"));
   }

   {
      SipMessage inv;

      UnknownHeaderType h_Foo("foo");
      UnknownHeaderType h_Bar("bar");

      inv.header(h_Foo);
      inv.header(h_Bar).push_back(StringCategory("bar1"));
      inv.header(h_Bar).push_back(StringCategory("bar2"));

      //inv.header(h_Vias);
      inv.header(h_To);
      
      inv.header(h_RequestLine) = RequestLine(INVITE);
      inv.header(h_RequestLine).uri() = Uri("sip:bob@biloxi.com");
      inv.header(h_To) = NameAddr("sip:bob@biloxi.com");
      inv.header(h_From) = NameAddr("Alice <sip:alice@atlanta.com>;tag=1928301774");
      inv.header(h_CallId).value() = "314159";
      inv.header(h_CSeq).sequence() = 14;
      inv.header(h_CSeq).method() = INVITE;

      auto_ptr<Contents> pc(new PlainContents("here is some plain ol' contents"));
      inv.setContents(pc);

      resipCerr << inv.header(h_ContentType).type() << endl;
      assert(inv.header(h_ContentType).type() == "text");
      assert(inv.header(h_ContentType).subType() == "plain");

      assert(!inv.exists(h_ContentLength));

      assert(inv.getContents());
      assert(dynamic_cast<PlainContents*>(inv.getContents()));
      assert(dynamic_cast<PlainContents*>(inv.getContents())->text() == "here is some plain ol' contents");

      Data d;
      {
         DataStream s(d);
         inv.encode(s);
      }
      
      resipCerr << "!! " << d;
      assert(d == ("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                   "To: <sip:bob@biloxi.com>\r\n"
                   "From: \"Alice\"<sip:alice@atlanta.com>;tag=1928301774\r\n"
                   "Call-ID: 314159\r\n"
                   "CSeq: 14 INVITE\r\n"
                   "Content-Type: text/plain\r\n"
//                   "foo: \r\n"
                   "bar: bar1\r\n"
                   "bar: bar2\r\n"
                   "Content-Length: 31\r\n"
                   "\r\n"
                   "here is some plain ol' contents"));
   }
   
   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: \"Bob\"<sip:bob@biloxi.com>\r\n"
               "From: \"Alice\"<sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      assert(msg->exists(h_ContentType));
      assert(msg->exists(h_ContentLength));
      assert(msg->header(h_ContentLength).value() == 150);

      Contents* body = msg->getContents();

      assert(body != 0);
      SdpContents* sdp = dynamic_cast<SdpContents*>(body);
      assert(sdp != 0);

      assert(sdp->session().version() == 0);
      assert(sdp->session().origin().user() == "alice");
      assert(!sdp->session().media().empty());
      assert(sdp->session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");

      msg->encode(resipCerr);
   }

   {
      InfoLog(<< "Test SipMessage::releaseContents; no contents");

      SipMessage msg;
      assert(msg.getContents() == 0);
      auto_ptr<Contents> old = msg.releaseContents();
      assert(old.get() == 0);
      assert(msg.getContents() == 0);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      assert(msg->exists(h_ContentType));
      auto_ptr<Contents> abody = msg->releaseContents();
      Contents* body = abody.release();

      assert(body != 0);
      SdpContents* sdp = dynamic_cast<SdpContents*>(body);
      assert(sdp != 0);

      assert(sdp->session().version() == 0);
      assert(sdp->session().origin().user() == "alice");
      assert(!sdp->session().media().empty());
      assert(sdp->session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");

      delete sdp;

      assert(msg->getContents() == 0);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Foobie-Blech: it is not a glass paperweight\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(!msg->header(UnknownHeaderType("Foobie-Blech")).empty());
      assert(msg->header(UnknownHeaderType("Foobie-Blech")).front().value() == "it is not a glass paperweight");
      
      Contents* body = msg->getContents();

      assert(body != 0);
      SdpContents* sdp = dynamic_cast<SdpContents*>(body);
      assert(sdp != 0);

      assert(sdp->session().version() == 0);
      assert(sdp->session().origin().user() == "alice");
      assert(!sdp->session().media().empty());
      assert(sdp->session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");

      msg->encode(resipCerr);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Foobie-Blech: \r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      msg->header(UnknownHeaderType("Foobie-Blech")).empty();
      //assert(!msg->header(UnknownHeaderType("Foobie-Blech")).empty());
      //assert(msg->header(UnknownHeaderType("Foobie-Blech")).front().value() == "");
      msg->encode(resipCerr);
   }

   {
      const char* b = "shared buffer";
      HeaderFieldValue h1(b, strlen(b));
      HeaderFieldValue h2(h1);
   }

   {
      const char *txt = 
         ("SIP/2.0 200\r\n"
          "To: <sip:ext102@squamish.gloo.net:5060>;tag=8be36d98\r\n"
          "From: <sip:ext102@squamish.gloo.net:5060>;tag=38810b6d\r\n"
          "Call-ID: a6aea86d75a6bb45\r\n"
          "CSeq: 2 REGISTER\r\n"
          "Contact: <sip:ext102@whistler.gloo.net:6064>;expires=63\r\n"
          "Via: SIP/2.0/UDP whistler.gloo.net:6064;rport=6064;received=192.168.2.220;branch=z9hG4bK-kcD23-4-1\r\n"
          "Content-Length: 0\r\n"
          "\r\n");
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      resipCerr << msg->header(h_Contacts).front().param(p_expires) << endl;
      assert(msg->header(h_Contacts).front().param(p_expires) == 63);
   }

   {
      resipCerr << "test backward compatible expires parameter" << endl;
      const char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>;expires=\"Sat, 01 Dec 2040 16:00:00 GMT\";foo=bar\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));
      resipCerr << message1->header(h_Contacts).front().param(p_expires) << endl;
      assert(message1->header(h_Contacts).front().param(p_expires) == 3600);
      assert(message1->header(h_Contacts).front().param(UnknownParameterType("foo")) == "bar");
   }

   {
      resipCerr << "test header copying between unparsed messages" << endl;
      const char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 7200\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));
      auto_ptr<SipMessage> r(Helper::makeResponse(*message1, 100));
      r->encode(resipCerr);

      const char *txt2 = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfirst\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=ssecond\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sthird\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfourth\r\n"
                    "Max-Forwards: 7\r\n"
                    "To: Speedy <sip:speedy@biloxi.com>\r\n"
                    "From: Speedy <sip:speedy@biloxi.com>;tag=88888\r\n"
                    "Call-ID: 88888@8888\r\n"
                    "CSeq: 6281 REGISTER\r\n"
                    "Contact: <sip:speedy@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 2700\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      // copy over everything
      message1->header(h_RequestLine) = message2->header(h_RequestLine);
      message1->header(h_Vias) = message2->header(h_Vias);
      message1->header(h_MaxForwards) = message2->header(h_MaxForwards);
      message1->header(h_To) = message2->header(h_To);
      message1->header(h_From) = message2->header(h_From);
      message1->header(h_CallId) = message2->header(h_CallId);
      message1->header(h_CSeq) = message2->header(h_CSeq);
      message1->header(h_Contacts) = message2->header(h_Contacts);
      message1->header(h_Expires) = message2->header(h_Expires);
      message1->header(h_ContentLength) = message2->header(h_ContentLength);
      
      assert(message1->header(h_To).uri().user() == "speedy");
      assert(message1->header(h_From).uri().user() == "speedy");
      assert(message1->header(h_MaxForwards).value() == 7);
      assert(message1->header(h_Contacts).empty() == false);
      assert(message1->header(h_CallId).value() == "88888@8888");
      assert(message1->header(h_CSeq).sequence() == 6281);
      assert(message1->header(h_CSeq).method() == REGISTER);
      assert(message1->header(h_Vias).empty() == false);
      assert(message1->header(h_Vias).size() == 4);
      assert(message1->header(h_Expires).value() == 2700);
      assert(message1->header(h_ContentLength).value() == 0);
      resipCerr << "Port: " << message1->header(h_RequestLine).uri().port() << endl;
      resipCerr << "AOR: " << message1->header(h_RequestLine).uri().getAor() << endl;
      assert(message1->header(h_RequestLine).uri().getAor() == "registrar.ixolib.com");
   }

   {
      resipCerr << "test header copying between parsed messages" << endl;
      resipCerr << " should NOT COPY any HeaderFieldValues" << endl;
      const char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 7200\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));

      // parse it
      message1->header(h_RequestLine).getMethod();
      for (NameAddrs::iterator i = message1->header(h_Contacts).begin();
           i != message1->header(h_Contacts).end(); i++)
      {
         i->uri();
      }

      for (Vias::iterator i = message1->header(h_Vias).begin();
           i != message1->header(h_Vias).end(); i++)
      {
         i->sentPort();
      }

      message1->header(h_To).uri().user();
      message1->header(h_From).uri().user();
      message1->header(h_MaxForwards).value();
      message1->header(h_Contacts).empty();
      message1->header(h_CallId).value();
      message1->header(h_CSeq).sequence();
      message1->header(h_CSeq).method();
      message1->header(h_Vias).empty();
      message1->header(h_Vias).size();
      message1->header(h_Expires).value();
      message1->header(h_ContentLength).value();

      const char *txt2 = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sfirst\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=ssecond\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sthird\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sfourth\r\n"
                    "Max-Forwards: 7\r\n"
                    "To: Speedy <sip:speedy@biloxi.com>\r\n"
                    "From: Belle <sip:belle@biloxi.com>;tag=88888\r\n"
                    "Call-ID: 88888@8888\r\n"
                    "CSeq: 6281 REGISTER\r\n"
                    "Contact: <sip:belle@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 2700\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      assert(message2->header(h_RequestLine).getMethod() == REGISTER);
      assert(message2->header(h_To).uri().user() == "speedy");
      assert(message2->header(h_From).uri().user() == "belle");
      assert(message2->header(h_MaxForwards).value() == 7);
      for (NameAddrs::iterator i = message2->header(h_Contacts).begin();
           i != message2->header(h_Contacts).end(); i++)
      {
         i->uri();
      }

      for (Vias::iterator i = message2->header(h_Vias).begin();
           i != message2->header(h_Vias).end(); i++)
      {
         assert(i->sentPort() == 5061);
      }
      assert(message2->header(h_CallId).value() == "88888@8888");
      assert(message2->header(h_CSeq).sequence() == 6281);
      assert(message2->header(h_CSeq).method() == REGISTER);
      assert(message2->header(h_Vias).empty() == false);
      assert(message2->header(h_Vias).size() == 4);
      assert(message2->header(h_Expires).value() == 2700);
      assert(message2->header(h_ContentLength).value() == 0);

      // copy over everything
      message1->header(h_RequestLine) = message2->header(h_RequestLine);
      message1->header(h_Vias) = message2->header(h_Vias);
      message1->header(h_MaxForwards) = message2->header(h_MaxForwards);
      message1->header(h_To) = message2->header(h_To);
      message1->header(h_From) = message2->header(h_From);
      message1->header(h_CallId) = message2->header(h_CallId);
      message1->header(h_CSeq) = message2->header(h_CSeq);
      message1->header(h_Contacts) = message2->header(h_Contacts);
      message1->header(h_Expires) = message2->header(h_Expires);
      message1->header(h_ContentLength) = message2->header(h_ContentLength);

      assert(message1->header(h_To).uri().user() == "speedy");
      assert(message1->header(h_From).uri().user() == "belle");
      assert(message1->header(h_MaxForwards).value() == 7);
      assert(message1->header(h_Contacts).empty() == false);
      assert(message1->header(h_CallId).value() == "88888@8888");
      assert(message1->header(h_CSeq).sequence() == 6281);
      assert(message1->header(h_CSeq).method() == REGISTER);
      assert(message1->header(h_Vias).empty() == false);
      assert(message1->header(h_Vias).size() == 4);
      assert(message1->header(h_Expires).value() == 2700);
      assert(message1->header(h_ContentLength).value() == 0);
      assert(message1->header(h_RequestLine).uri().getAor() == "registrar.ixolib.com");
   }

   {
      resipCerr << "test unparsed message copy" << endl;
      const char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Contact: <sip:qoq@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      SipMessage copy(*message);
      copy.encode(resipCerr);
      resipCerr << endl;
   }
   
   {
      resipCerr << "test header creation" << endl;
      SipMessage message;

      message.header(h_CSeq).sequence() = 123456;
      assert(message.header(h_CSeq).sequence() == 123456);

      message.header(h_To).uri().user() = "speedy";
      assert(message.header(h_To).uri().user() == "speedy");
      
      message.encode(resipCerr);

   }
   
   {
      resipCerr << "test multiheaders access" << endl;

      const char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Contact: <sip:qoq@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      resipCerr << "Encode from unparsed: " << endl;
      message->encode(resipCerr);

      assert(message->header(h_To).uri().user() == "bob");
      assert(message->header(h_From).uri().user() == "bob");
      assert(message->header(h_MaxForwards).value() == 70);
      assert(message->header(h_Contacts).empty() == false);
      assert(message->header(h_CallId).value() == "843817637684230@998sdasdh09");
      assert(message->header(h_CSeq).sequence() == 1826);
      assert(message->header(h_CSeq).method() == REGISTER);
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).size() == 5);
      assert(message->header(h_Expires).value() == 7200);
      assert(message->header(h_ContentLength).value() == 0);
      assert(message->header(h_RequestLine).uri().getAor() == "registrar.biloxi.com");
      
      resipCerr << "Encode from parsed: " << endl;
      message->encode(resipCerr);

      message->header(h_Contacts).front().uri().user() = "jason";

      resipCerr << "Encode after messing: " << endl;
      message->encode(resipCerr);

      SipMessage copy(*message);
      assert(copy.header(h_To).uri().user() == "bob");
      assert(copy.header(h_From).uri().user() == "bob");
      assert(copy.header(h_MaxForwards).value() == 70);
      assert(copy.header(h_Contacts).empty() == false);
      assert(copy.header(h_CallId).value() == "843817637684230@998sdasdh09");
      assert(copy.header(h_CSeq).sequence() == 1826);
      assert(copy.header(h_CSeq).method() == REGISTER);
      assert(copy.header(h_Vias).empty() == false);
      assert(copy.header(h_Vias).size() == 5);
      assert(copy.header(h_Expires).value() == 7200);
      assert(copy.header(h_ContentLength).value() == 0);
      resipCerr << "RequestLine Uri AOR = " << copy.header(h_RequestLine).uri().getAor() << endl;
      assert(copy.header(h_RequestLine).uri().getAor() == "registrar.biloxi.com");


      resipCerr << "Encode after copying: " << endl;
      copy.encode(resipCerr);
   }
   
   {
      resipCerr << "test callId access" << endl;

      const char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      message->encode(resipCerr);
      
      //Data v = message->header(h_CallId).value();
      assert(message->header(h_CallId).value() == "843817637684230@998sdasdh09");
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //resipCerr << bar.getMethod() << endl;
   }
   
   {
      const char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248;xmobility=hobble\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
          
      Data v = message->header(h_CallId).value();
      resipCerr << "Call-ID is " << v << endl;

      message->encode(resipCerr);
  
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //resipCerr << bar.getMethod() << endl;
   }

   {
      
      const char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248;mobility=\"hobble\"\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      assert(message->getRawHeader(Headers::From));
      assert(&message->header(h_From));
      assert(message->header(h_From).exists(p_tag) == true);
      assert(message->header(h_From).exists(p_mobility) == true);
      assert(message->header(h_From).param(p_tag) == "456248");
      assert(message->header(h_From).param(p_mobility) == "hobble");

      message->encode(resipCerr);
  
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //resipCerr << bar.getMethod() << endl;
   }

   {
      resipCerr << "first REGISTER in torture test" << endl;
      
      const char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->exists(h_To));
      assert(message->header(h_To).uri().user() == "user");
      assert(message->header(h_To).uri().host() == "company.com");
      assert(message->header(h_To).uri().exists(p_tag_ext) == false);

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().user() == "user");
      assert(message->header(h_From).uri().host() == "company.com");
      assert(message->header(h_From).param(p_tag) == "3411345");

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 8);
      assert(message->header(h_MaxForwards).exists(p_tag_ext) == false);

      assert(message->exists(h_Contacts));
      assert(message->header(h_Contacts).empty() == false);
      assert(message->header(h_Contacts).front().uri().user() == "user");
      assert(message->header(h_Contacts).front().uri().host() == "host.company.com");
      assert(message->header(h_Contacts).front().uri().port() == 0);

      assert(message->exists(h_CallId));
      assert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      assert(message->exists(h_CSeq));
      assert(message->header(h_CSeq).sequence() == 8);
      assert(message->header(h_CSeq).method() == REGISTER);

      assert(message->exists(h_Vias));
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).front().protocolName() == "SIP");
      assert(message->header(h_Vias).front().protocolVersion() == "2.0");
      assert(message->header(h_Vias).front().transport() == "UDP");
      assert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message->header(h_Vias).front().sentPort() == 0);

      assert(message->exists(h_Expires));
      assert(message->header(h_Expires).value() == 353245);

      resipCerr << "Headers::Expires enum = " << h_Expires.getTypeNum() << endl;
      
      message->encode(resipCerr);
   }

   {
      resipCerr << "first REGISTER in torture test" << endl;
      
      const char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_MaxForwards).value() == 8);
      message->getRawHeader(Headers::MaxForwards)->getParserContainer()->encode(Headers::getHeaderName(Headers::MaxForwards), resipCerr) << endl;
   }

   {
      resipCerr << "response to REGISTER" << endl;
      
      const char *txt = ("SIP/2.0 100 Trying\r\n"
                   "To: sip:localhost:5070\r\n"
                   "From: sip:localhost:5070;tag=73483366\r\n"
                   "Call-ID: 51dcb07418a21008e0ba100800000000\r\n"
                   "CSeq: 1 REGISTER\r\n"
                   "Via: SIP/2.0/UDP squamish.gloo.net:5060;branch=z9hG4bKff5c491951e40f08\r\n"
                   "Content-Length: 0\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_To).uri().host() == "localhost");
   }
   {
      NameAddr me;
      me.uri().host() = "localhost";
      me.uri().port() = 5070;
      //auto_ptr<SipMessage> msg(Helper::makeRegister(me, me));
      auto_ptr<SipMessage> msg(Helper::makeRegister(me, me, me));
      resipCerr << "encoded=" << *msg << endl;
   }
   {
      const char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      message->header(h_RequestLine).uri();
      auto_ptr<SipMessage> copy(new SipMessage(*message));
      assert(message->header(h_RequestLine).getMethod() == copy->header(h_RequestLine).getMethod());
   }
   {
      const char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Security-Client: ipsec-ike;d-alg=md5;q=0.1\r\n"
                   "Security-Server: tls;q=0.2;d-qop=verify\r\n"
                   "Security-Verify: tls;q=0.2;d-ver=\"0000000000000000000000000000abcd\"\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      assert(message->header(h_SecurityClients).front().value() == "ipsec-ike");
      assert(message->header(h_SecurityClients).front().param(p_dAlg) == "md5");
      assert(message->header(h_SecurityClients).front().param(p_q) == 100);
#ifndef RESIP_FIXED_POINT
      assert(message->header(h_SecurityClients).front().param(p_q) == double(0.1));
      assert(message->header(h_SecurityClients).front().param(p_q) == 0.1);
      assert(message->header(h_SecurityClients).front().param(p_q) == float(0.1));
#endif

      assert(message->header(h_SecurityServers).front().value() == "tls");
      assert(message->header(h_SecurityServers).front().param(p_dQop) == "verify");
#ifndef RESIP_FIXED_POINT
      assert(message->header(h_SecurityServers).front().param(p_q) == double(0.2));
      assert(message->header(h_SecurityServers).front().param(p_q) == 0.2f);
#endif
      assert(message->header(h_SecurityServers).front().param(p_q) == 200);

      assert(message->header(h_SecurityVerifies).front().value() == "tls");
      assert(message->header(h_SecurityVerifies).front().param(p_dVer) == "0000000000000000000000000000abcd");
#ifndef RESIP_FIXED_POINT
      assert(message->header(h_SecurityVerifies).front().param(p_q) == double(0.2));
      assert(message->header(h_SecurityVerifies).front().param(p_q) == 0.2f);
#endif
      assert(message->header(h_SecurityVerifies).front().param(p_q) == 200);

      assert(message->exists(h_AllowEvents) == false);
   }

   {
      const char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      const char *txt2 = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Security-Client: ipsec-ike;d-alg=md5;q=0.1\r\n"
                   "Security-Server: tls;q=0.2;d-qop=verify\r\n"
                   "Security-Verify: tls;q=0.2;d-ver=\"0000000000000000000000000000abcd\"\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      {
         Token sec;
         sec.value() = "ipsec-ike";
         sec.param(p_dAlg) = "md5";
         sec.param(p_q) = 100;
         message->header(h_SecurityClients).push_back(sec);
      }
      {
         Token sec;
         sec.value() = "tls";
         sec.param(p_q) = 200;
         sec.param(p_dQop) = "verify";
         message->header(h_SecurityServers).push_back(sec);
      }
      {
         Token sec;
         sec.value() = "tls";
         sec.param(p_q) = 200;
         sec.param(p_dVer) = "0000000000000000000000000000abcd";
         message->header(h_SecurityVerifies).push_back(sec);
      }

      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      Data msgEncoded;
      {
         DataStream s(msgEncoded);
         s << *message;
      }
      
      Data msg2Encoded;
      {
         DataStream s(msg2Encoded);
         s << *message2;
      }
      
      assert(msgEncoded == msg2Encoded);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Accept: \r\n"
               "Foobie-Blech: \r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(msg->exists(h_Accepts));
      assert(msg->header(h_Accepts).empty());
      
      assert(msg->exists(UnknownHeaderType("Foobie-Blech")));
      assert(msg->header(UnknownHeaderType("Foobie-Blech")).empty());
   }
   
   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Accept: \r\n"
               "Foobie-Blech: \r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "FooBarBaz: yetmorestuff\r\n"
               "FooBar: morestuff\r\n"
               "Foo: stuff\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(msg->exists(h_Accepts));
      assert(msg->header(h_Accepts).empty());
      
      assert(msg->exists(UnknownHeaderType("Foobie-Blech")));
      assert(msg->header(UnknownHeaderType("Foobie-Blech")).empty());

      assert(msg->exists(UnknownHeaderType("Foo")));
      assert(msg->header(UnknownHeaderType("Foo")).size()==1);

      assert(msg->exists(UnknownHeaderType("FooBar")));
      assert(msg->header(UnknownHeaderType("FooBar")).size()==1);

      assert(msg->exists(UnknownHeaderType("FooBarBaz")));
      assert(msg->header(UnknownHeaderType("FooBarBaz")).size()==1);
   }

   {
      resip::Data invBuf;
      {
         resip::DataStream ds(invBuf);

         ds << "INVITE sip:7003@192.122.24.94:20060?REQUIRE=replaces SIP/2.0\r\n";
         ds << "Via: SIP/2.0/UDP 192.122.24.109;branch=z9hG4bKfc4a20db5EC58FB8\r\n";
         ds << "From: \"7006 601\" <sip:7006@scsuk.europe.nortel.com>;tag=29C49D21-D9FDBDBC\r\n";
         ds << "To: <sip:7003@192.122.24.94:20060?REQUIRE=replaces>\r\n";
         ds << "CSeq: 1 INVITE\r\n";
         ds << "Call-ID: a9ad18fd-5fd391bf-309febea@192.122.24.109\r\n";
         ds << "Contact: <sip:7006@192.122.24.109>\r\n";
         ds << "Allow: INVITE, ACK, BYE, CANCEL, OPTIONS, INFO, MESSAGE, SUBSCRIBE, NOTIFY, PRACK, UPDATE, REFER\r\n";
         ds << "User-Agent: PolycomSoundPointIP-SPIP_601-UA/2.1.1.0052\r\n";
         ds << "Supported: 100rel,replaces\r\n";
         ds << "Allow-Events: talk,hold,conference\r\n";
         ds << "Replaces: MDQ5MDJmZGEyYTkyNTBhZjBhNDg0NjUyMzE1MTc2Mjk.;to-tag=cf71d002;from-tag=1394930945\r\n";
         ds << "Referred-By: <sip:7776@scsuk.europe.nortel.com>\r\n";
         ds << "Max-Forwards: 70\r\n";
         ds << "Content-Type: application/sdp\r\n";
         ds << "Content-Length: 253\r\n";
         ds << "\r\n";
         ds << "v=0\r\n";
         ds << "o=- 1210601497 1210601497 IN IP4 192.122.24.109\r\n";
         ds << "s=Polycom IP Phone\r\n";
         ds << "c=IN IP4 192.122.24.109\r\n";
         ds << "t=0 0\r\n";
         ds << "m=audio 2224 RTP/AVP 0 8 18 101\r\n";
         ds << "a=sendrecv\r\n";
         ds << "a=rtpmap:0 PCMU/8000\r\n";
         ds << "a=rtpmap:8 PCMA/8000\r\n";
         ds << "a=rtpmap:18 G729/8000\r\n";
         ds << "a=rtpmap:101 telephone-event/8000\r\n";
      }

      auto_ptr<SipMessage> testMsg(TestSupport::makeMessage(invBuf));
      assert(testMsg->header(h_RequestLine).isWellFormed());
      assert(testMsg->header(h_RequestLine).method() == INVITE);
      assert(testMsg->header(h_RequestLine).getSipVersion() == "SIP/2.0");
      assert(testMsg->header(h_RequestLine).uri().hasEmbedded());
      SipMessage& embeddedMsg = testMsg->header(h_RequestLine).uri().embedded();
      assert(embeddedMsg.exists(h_Requires));
      assert(embeddedMsg.header(h_Requires).find(Token(Symbols::Replaces)));
      assert(testMsg->header(h_To).uri().hasEmbedded());
      SipMessage& embeddedMsg2 = testMsg->header(h_To).uri().embedded();
      assert(embeddedMsg2.exists(h_Requires));
      assert(embeddedMsg2.header(h_Requires).find(Token(Symbols::Replaces)));
   }

   {
      Data txt(
         "GET / HTTP/1.1\r\n"
         "Upgrade: websocket\r\n"
         "Connection: Upgrade\r\n"
         "Host: localhost\r\n"
         "Origin: http://localhost\r\n"
         "Sec-WebSocket-Protocol: sip\r\n"
         "Pragma: no-cache\r\n"
         "Cache-Control: no-cache\r\n"
         "Sec-WebSocket-Key: rFi6Qbjr0EmH04nUqfCAKQ==\r\n"
         "Sec-WebSocket-Version: 13\r\n"
         "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits, x-webkit-deflate-frame\r\n"
         "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2062.120 Safari/537.36\r\n"
         "\r\n" );

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(msg->header(h_RequestLine).unknownMethodName() == "GET");
      assert(msg->header(h_RequestLine).uri().path() == "/");
      assert( msg->header(h_ContentLength).value() == 0 );
   }

   {
      Data txt(
         "GET /;p1=123;p2=456 HTTP/1.1\r\n"
         "Upgrade: websocket\r\n"
         "Connection: Upgrade\r\n"
         "Host: localhost\r\n"
         "Origin: http://localhost\r\n"
         "Sec-WebSocket-Protocol: sip\r\n"
         "Pragma: no-cache\r\n"
         "Cache-Control: no-cache\r\n"
         "Sec-WebSocket-Key: rFi6Qbjr0EmH04nUqfCAKQ==\r\n"
         "Sec-WebSocket-Version: 13\r\n"
         "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits, x-webkit-deflate-frame\r\n"
         "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2062.120 Safari/537.36\r\n"
         "\r\n" );

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(msg->header(h_RequestLine).unknownMethodName() == "GET");
      Uri& uri = msg->header(h_RequestLine).uri();
      assert(uri.path() == "/");
      assert(uri.param(UnknownParameterType("p1")) == Data("123"));
      assert(uri.param(UnknownParameterType("p2")) == Data("456"));

      assert( msg->header(h_ContentLength).value() == 0 );
   }

   resipCerr << "\nTEST OK" << endl;
   return 0;
}

/* vim: softtabstop=3:shiftwidth=3:expandtab */

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
