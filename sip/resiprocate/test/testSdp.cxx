#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/SdpContents.hxx"
#include "sip2/sipstack/HeaderFieldValue.hxx"

#include <iostream>
#include "TestSupport.hxx"
#include "tassert.h"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP

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
    tassert_init(4);
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
      
      tassert(sdp.getSession().getVersion() == 0);
      tassert(sdp.getSession().getOrigin().getUser() == "alice");
      tassert(!sdp.getSession().getMedia().empty());
      
      //this fails, but should probably not parse(t before c not in sdp)
      tassert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
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
      tassert(sdp.getSession().getVersion() == 0);
      tassert(sdp.getSession().getOrigin().getUser() == "UserA");
      tassert(sdp.getSession().getOrigin().getSessionId() == "2890844526");
      tassert(sdp.getSession().getOrigin().getVersion() == "2890844527");
      tassert(sdp.getSession().getOrigin().getAddressType() == SdpContents::IP4);
      tassert(sdp.getSession().getOrigin().getAddress() == "here.com");

      tassert(sdp.getSession().getName() == "Session SDP");

      tassert(sdp.getSession().getConnection().getAddressType() == SdpContents::IP4);
      tassert(sdp.getSession().getConnection().getAddress() == "pc33.atlanta.com");
      tassert(sdp.getSession().getConnection().getTTL() == 0);

      tassert(sdp.getSession().getTimes().front().getStart() == 5);
      tassert(sdp.getSession().getTimes().front().getStop() == 17);
      tassert(sdp.getSession().getTimes().front().getRepeats().empty());
      tassert(sdp.getSession().getTimezones().getAdjustments().empty());

      tassert(sdp.getSession().getMedia().front().getName() == "audio");
      tassert(sdp.getSession().getMedia().front().getPort() == 49172);
      tassert(sdp.getSession().getMedia().front().getMulticast() == 1);
      tassert(sdp.getSession().getMedia().front().getProtocol() == "RTP/AVP");
      tassert(sdp.getSession().getMedia().front().getFormats().front() == "0");

      tassert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
      tassert(sdp.getSession().getMedia().front().exists("fuzzy") == false);
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
      tassert(sdp.getSession().getVersion() == 0);
      tassert(sdp.getSession().getOrigin().getUser() == "CiscoSystemsSIP-GW-UserAgent");
      tassert(sdp.getSession().getOrigin().getSessionId() == "3559");
      tassert(sdp.getSession().getOrigin().getVersion() == "3228");
      tassert(sdp.getSession().getOrigin().getAddressType() == SdpContents::IP4);
      tassert(sdp.getSession().getOrigin().getAddress() == "192.168.2.122");

      tassert(sdp.getSession().getName() == "SIP Call");

      tassert(sdp.getSession().getConnection().getAddressType() == SdpContents::IP4);
      tassert(sdp.getSession().getConnection().getAddress() == "192.168.2.122");
      tassert(sdp.getSession().getConnection().getTTL() == 0);

      tassert(sdp.getSession().getTimes().front().getStart() == 0);
      tassert(sdp.getSession().getTimes().front().getStop() == 0);
      tassert(sdp.getSession().getTimes().front().getRepeats().empty());
      tassert(sdp.getSession().getTimezones().getAdjustments().empty());

      tassert(sdp.getSession().getMedia().front().getName() == "audio");
      tassert(sdp.getSession().getMedia().front().getPort() == 17124);
      tassert(sdp.getSession().getMedia().front().getMulticast() == 1);
      tassert(sdp.getSession().getMedia().front().getProtocol() == "RTP/AVP");
      tassert(sdp.getSession().getMedia().front().getFormats().front() == "18");

      tassert(sdp.getSession().getMedia().front().getValue("rtpmap") == "18 G729/8000");
      tassert(sdp.getSession().getMedia().front().exists("fuzzy") == false);
      tassert_verify(3);
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

      tassert(sdp.getSession().getVersion() == 0);
      tassert(sdp.getSession().getOrigin().getUser() == "alice");
      tassert(!sdp.getSession().getMedia().empty());
      //this fails, but should probably not parse(t before c not in sdp)
      tassert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
      tassert_verify(4);
   }
   tassert_report();
   


}

