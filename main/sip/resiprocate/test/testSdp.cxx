#include "sip2/sipstack/SdpContents.hxx"
#include "sip2/sipstack/HeaderFieldValue.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      Data txt("v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      SdpContents sdp(&hfv);

      assert(sdp.getSession().getVersion() == 0);
      assert(sdp.getSession().getOrigin().getUser() == "alice");
      assert(!sdp.getSession().getMedia().empty());
      //this fails, but should probably not parse(t before c not in sdp)
      assert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
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
          "\r\n"
            );

      HeaderFieldValue hfv(txt, strlen(txt));
      SdpContents sdp(&hfv);
      
      assert(sdp.getSession().getVersion() == 0);
      assert(sdp.getSession().getOrigin().getUser() == "UserA");
      assert(sdp.getSession().getOrigin().getSessionId() == "2890844526");
      assert(sdp.getSession().getOrigin().getVersion() == "2890844527");
      assert(sdp.getSession().getOrigin().getAddressType() == SdpContents::IP4);
      assert(sdp.getSession().getOrigin().getAddress() == "here.com");

      assert(sdp.getSession().getName() == "Session SDP");

      assert(sdp.getSession().getConnection().getAddressType() == SdpContents::IP4);
      assert(sdp.getSession().getConnection().getAddress() == "pc33.atlanta.com");
      assert(sdp.getSession().getConnection().getTTL() == 0);

      assert(sdp.getSession().getTimes().front().getStart() == 5);
      assert(sdp.getSession().getTimes().front().getStop() == 17);
      assert(sdp.getSession().getTimes().front().getRepeats().empty());
      assert(sdp.getSession().getTimezones().getAdjustments().empty());

      assert(sdp.getSession().getMedia().front().getName() == "audio");
      assert(sdp.getSession().getMedia().front().getPort() == 49172);
      assert(sdp.getSession().getMedia().front().getMulticast() == 1);
      assert(sdp.getSession().getMedia().front().getProtocol() == "RTP/AVP");
      assert(sdp.getSession().getMedia().front().getFormats().front() == "0");

      assert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
      assert(sdp.getSession().getMedia().front().exists("fuzzy") == false);
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
      SdpContents sdp(&hfv);
      
      assert(sdp.getSession().getVersion() == 0);
      assert(sdp.getSession().getOrigin().getUser() == "CiscoSystemsSIP-GW-UserAgent");
      assert(sdp.getSession().getOrigin().getSessionId() == "3559");
      assert(sdp.getSession().getOrigin().getVersion() == "3228");
      assert(sdp.getSession().getOrigin().getAddressType() == SdpContents::IP4);
      assert(sdp.getSession().getOrigin().getAddress() == "192.168.2.122");

      assert(sdp.getSession().getName() == "SIP Call");

      assert(sdp.getSession().getConnection().getAddressType() == SdpContents::IP4);
      assert(sdp.getSession().getConnection().getAddress() == "192.168.2.122");
      assert(sdp.getSession().getConnection().getTTL() == 0);

      assert(sdp.getSession().getTimes().front().getStart() == 0);
      assert(sdp.getSession().getTimes().front().getStop() == 0);
      assert(sdp.getSession().getTimes().front().getRepeats().empty());
      assert(sdp.getSession().getTimezones().getAdjustments().empty());

      assert(sdp.getSession().getMedia().front().getName() == "audio");
      assert(sdp.getSession().getMedia().front().getPort() == 17124);
      assert(sdp.getSession().getMedia().front().getMulticast() == 1);
      assert(sdp.getSession().getMedia().front().getProtocol() == "RTP/AVP");
      assert(sdp.getSession().getMedia().front().getFormats().front() == "18");

      assert(sdp.getSession().getMedia().front().getValue("rtpmap") == "18 G729/8000");
      assert(sdp.getSession().getMedia().front().exists("fuzzy") == false);
   }

   cerr << "All OK" << endl;

   cerr << "Except this will assert." << endl;
   {
      Data txt("v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "t=0 0\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      SdpContents sdp(&hfv);

      assert(sdp.getSession().getVersion() == 0);
      assert(sdp.getSession().getOrigin().getUser() == "alice");
      assert(!sdp.getSession().getMedia().empty());
      //this fails, but should probably not parse(t before c not in sdp)
      assert(sdp.getSession().getMedia().front().getValue("rtpmap") == "0 PCMU/8000");
   }


}

