#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/HeaderFieldValue.hxx"

#include <iostream>
#include "TestSupport.hxx"
#include "tassert.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP

int
main(int argc, char* argv[])
{
    Log::Level l = Log::DEBUG;
    
    if (argc > 1)
    {
        switch(*argv[1])
        {
            case 'd': l = Log::DEBUG;
                break;
            case 'i': l = Log::INFO;
                break;
            case 's': l = Log::DEBUG_STACK;
                break;
            case 'c': l = Log::CRIT;
                break;
        }
        
    }
    
    Log::initialize(Log::COUT, l, argv[0]);
    CritLog(<<"Test Driver Starting");

    {
       Data txt("v=0\r\n"
                "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
                "s=X-Lite\r\n"
                "c=IN IP4 192.168.2.15\r\n"
                "t=0 0\r\n"
                "m=audio 8000 RTP/AVP 8 3 98 97 101\r\n"
                "a=rtpmap:8 pcma/8000\r\n"
                "a=rtpmap:3 gsm/8000\r\n"
                "a=rtpmap:98 iLBC\r\n"
                "a=rtpmap:97 speex/8000\r\n"
                "a=rtpmap:101 telephone-event/8000\r\n"
                "a=fmtp:101 0-15\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);

      assert(sdp.session().media().front().codecs().size() == 4);
    }

    {
      Data txt("v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);

      HeaderFieldValue hfv2(txt.data(), txt.size());
      SdpContents sdp2(&hfv2, type);

      sdp.session();
      sdp2.session();

      Data sdpO = Data::from(sdp);
      sdp = sdp2;
      Data sdpO2 = Data::from(sdp);

      cerr << "!! " << sdp << endl;
      assert(sdpO == sdpO2);
    }
    
    tassert_init(5);
    {
      Data txt("v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);

      tassert_reset();
      
      tassert(sdp.session().version() == 0);
      tassert(sdp.session().origin().user() == "alice");
      tassert(!sdp.session().media().empty());
      
      //this fails, but should probably not parse(t before c not in sdp)
      tassert(sdp.session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");
      tassert_verify(1);
      
   }

   {
      const char* txt = 
         ("v=0\r\n"
          "o=UserA 2890844526 2890844527 IN IP4 here.com\r\n"
          "s=Session SDP\r\n"
          "c=IN IP4 pc33.atlanta.com\r\n"
          "t=5 17\r\n"
          "m=audio 49172 RTP/AVP 0\r\n"
          "a=rtpmap:0 PCMU/8000\r\n"
          "\r\n");

      HeaderFieldValue hfv(txt, strlen(txt));
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);
      tassert_reset();
      tassert(sdp.session().version() == 0);
      tassert(sdp.session().origin().user() == "UserA");
      tassert(sdp.session().origin().getSessionId() == 2890844526UL);
      tassert(sdp.session().origin().getVersion() == 2890844527UL);
      tassert(sdp.session().origin().getAddressType() == SdpContents::IP4);
      tassert(sdp.session().origin().getAddress() == "here.com");

      tassert(sdp.session().name() == "Session SDP");

      tassert(sdp.session().connection().getAddressType() == SdpContents::IP4);
      tassert(sdp.session().connection().getAddress() == "pc33.atlanta.com");
      tassert(sdp.session().connection().ttl() == 0);

      tassert(sdp.session().getTimes().front().getStart() == 5);
      tassert(sdp.session().getTimes().front().getStop() == 17);
      tassert(sdp.session().getTimes().front().getRepeats().empty());
      tassert(sdp.session().getTimezones().getAdjustments().empty());

      tassert(sdp.session().media().front().name() == "audio");
      tassert(sdp.session().media().front().port() == 49172);
      tassert(sdp.session().media().front().multicast() == 1);
      tassert(sdp.session().media().front().protocol() == "RTP/AVP");
      tassert(sdp.session().media().front().getFormats().front() == "0");

      tassert(sdp.session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");
      tassert(sdp.session().media().front().exists("fuzzy") == false);
      tassert_verify(2);
      
   }

   {
      const char* txt = ("v=0\r\n"
                         "o=CiscoSystemsSIP-GW-UserAgent 3559 3228 IN IP4 192.168.2.122\r\n"
                         "s=SIP Call\r\n"
                         "c=IN IP4 192.168.2.122\r\n"
                         "t=0 0\r\n"
                         "m=audio 17124 RTP/AVP 18\r\n"
                         "a=rtpmap:18 G729/8000\r\n");

      HeaderFieldValue hfv(txt, strlen(txt));
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);
      tassert_reset();
      tassert(sdp.session().version() == 0);
      tassert(sdp.session().origin().user() == "CiscoSystemsSIP-GW-UserAgent");
      tassert(sdp.session().origin().getSessionId() == 3559);
      tassert(sdp.session().origin().getVersion() == 3228);
      tassert(sdp.session().origin().getAddressType() == SdpContents::IP4);
      tassert(sdp.session().origin().getAddress() == "192.168.2.122");

      tassert(sdp.session().name() == "SIP Call");

      tassert(sdp.session().connection().getAddressType() == SdpContents::IP4);
      tassert(sdp.session().connection().getAddress() == "192.168.2.122");
      tassert(sdp.session().connection().ttl() == 0);

      tassert(sdp.session().getTimes().front().getStart() == 0);
      tassert(sdp.session().getTimes().front().getStop() == 0);
      tassert(sdp.session().getTimes().front().getRepeats().empty());
      tassert(sdp.session().getTimezones().getAdjustments().empty());

      tassert(sdp.session().media().front().name() == "audio");
      tassert(sdp.session().media().front().port() == 17124);
      tassert(sdp.session().media().front().multicast() == 1);
      tassert(sdp.session().media().front().protocol() == "RTP/AVP");
      tassert(sdp.session().media().front().getFormats().front() == "18");

      tassert(sdp.session().media().front().getValues("rtpmap").front() == "18 G729/8000");
      tassert(sdp.session().media().front().exists("fuzzy") == false);
      tassert_verify(3);
   }
   
   {
      tassert_reset();
      SdpContents sdp;
      unsigned long tm = 4058038202u;
      Data address("192.168.2.220");
      int port = 5061;
   
      unsigned long sessionId((unsigned long) tm);
   
      SdpContents::Session::Origin origin("-", sessionId, sessionId, SdpContents::IP4, address);
   
      SdpContents::Session session(0, origin, "VOVIDA Session");
      
      session.connection() = SdpContents::Session::Connection(SdpContents::IP4, address);
      session.addTime(SdpContents::Session::Time(tm, 0));
      
      SdpContents::Session::Medium medium("audio", port, 0, "RTP/AVP");
      medium.addFormat("0");
      medium.addFormat("101");
      
      medium.addAttribute("rtpmap", "0 PCMU/8000");
      medium.addAttribute("rtpmap", "101 telephone-event/8000");
      medium.addAttribute("ptime", "160");
      medium.addAttribute("fmtp", "101 0-11");
      
      session.addMedium(medium);
      
      sdp.session() = session;

      Data shouldBeLike("v=0\r\n"
                        "o=- 4058038202 4058038202 IN IP4 192.168.2.220\r\n"
                        "s=VOVIDA Session\r\n"
                        "c=IN IP4 192.168.2.220\r\n"
                        "t=4058038202 0\r\n"
                        "m=audio 5061 RTP/AVP 0 101\r\n"
                        "a=fmtp:101 0-11\r\n"
                        "a=ptime:160\r\n"
                        "a=rtpmap:0 PCMU/8000\r\n"
                        "a=rtpmap:101 telephone-event/8000\r\n");

      Data encoded(Data::from(sdp));

      std::cerr << "!! " << encoded;
      /**/assert(encoded == shouldBeLike);
      tassert_verify(4);
   }
   
   {
      Data txt("v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "t=0 0\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(&hfv, type);

      tassert(sdp.session().version() == 0);
      tassert(sdp.session().origin().user() == "alice");
      tassert(!sdp.session().media().empty());
      //this fails, but should probably not parse(t before c not in sdp)
      tassert(sdp.session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");
      tassert_verify(5);
   }
   tassert_report();
   


}

