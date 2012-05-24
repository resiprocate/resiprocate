#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "rutil/ParseBuffer.hxx"

#include <iostream>
#include "TestSupport.hxx"
#include "tassert.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
    Log::Level l = Log::Debug;
    
    if (argc > 1)
    {
        switch(*argv[1])
        {
            case 'd': l = Log::Debug;
                break;
            case 'i': l = Log::Info;
                break;
            case 's': l = Log::Stack;
                break;
            case 'c': l = Log::Crit;
                break;
        }
        
    }
    
    Log::initialize(Log::Cout, l, argv[0]);
    CritLog(<<"Test Driver Starting");

    {
       Data txt("v=0\r\n"  
                "o=VGW 1251901012 1251901012 IN IP4 10.1.83.143\r\n"
                "s=VGW\r\n"
                "c=IN IP4 10.1.83.143\r\n"
                "t=0 0\r\n"
                "a=sendrecv\r\n"
                "a=x-ActiveSpeaker:on\r\n"
                "m=audio 45894 RTP/AVP 103 \r\n"
                "a=rtpmap:103 ISAC/16000/1\r\n"
                "a=fmtp:103 mode=30, type=fixed, bitrate=32000\r\n"
                "a=silenceSupp:off - - - -\r\n");
       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
      
       assert(sdp.session().media().size() == 1);
	   assert(sdp.session().media().front().codecs().size() == 1);
       CritLog(<< "space at end of m line test passed");
    }

    {
       Data txt("v=0\r\n"  
		"o=- 333525334858460 333525334858460 IN IP4 192.168.0.156\r\n"
		"s=test123\r\n"
		"c=IN IP4 192.168.0.156\r\n"
		"t=4058038202 0\r\n"
		"m=audio 41466 RTP/AVP 0 101\r\n"
		"a=ptime:20\r\n"
		"a=rtpmap:0 PCMU/8000\r\n"
		"a=rtpmap:101 telephone-event/8000\r\n"
		"a=fmtp:101 0-11\r\n");

       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
      
       assert(sdp.session().media().size() == 1);
       resip::SdpContents::Session::Codec testCodec("PCMU", 8000, "", "1");
       for (std::list<resip::SdpContents::Session::Medium>::const_iterator i = sdp.session().media().begin(); i != sdp.session().media().end(); i++)
       {
          const std::list<resip::SdpContents::Session::Codec> &codecs = i->codecs();
          assert(testCodec == codecs.front());
       }

       //assert(sdp.session.getAttributes().count == 2);
       CritLog(<< "ftmp test: " << sdp);
    }

    {
       Data txt("v=0\r\n"  
		"o=- 333525334858460 333525334858460 IN IP4 192.168.0.156\r\n"
		"s=test123\r\n"
		"e=unknown@invalid.net\r\n"
		"p=+972 683 1000\r\n"
		"t=4058038202 0\r\n"
		"m=audio 41466 RTP/AVP 0 101\r\n"
		"c=IN IP4 192.168.0.156\r\n"
		"a=fmtp:101 0-11\r\n"
		"a=ptime:20\r\n"
		"a=rtpmap:101 telephone-event/8000\r\n");

       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
       assert(sdp.session().getPhones().size() == 1);
       assert(sdp.session().getEmails().size() == 1);

       assert(Data::from(sdp) == txt);
       
       CritLog(<< "Email + Phone Test Ok");
    }

    {
       Data txt("v=0\r\n"
                "o=ViPr 1 1 IN IP4 72.29.231.47\r\n"
                "s=eyeBeam\r\n"
                "i=\"q2\"<sip:q2@host1.marc.sipit.net>\r\n"
                "e=NoEmail@NoEmail.com\r\n"
                "t=0 0\r\n"
                "a=X-app:ViPr 11 ViPrTerminal\r\n"
                "a=X-GUID:4a8f41cc8a50_72.29.231.47_\r\n"
                "a=X-CollabStatus: CollabState_Idle\r\n"
                "m=audio 50958 RTP/AVP 0\r\n"
                "i=\"q2\"<sip:q2@host1.marc.sipit.net>\r\n"
                "c=IN IP4 72.29.231.47\r\n"
                "a=rtpmap:0 PCMU/8000/1\r\n"
                "a=sendrecv\r\n"
                "a=X-app:ViPr 11 ViPrTerminal\r\n"
                "m=video 0 RTP/AVP 32\r\n"
                "c=IN IP4 72.29.231.47\r\n");
       

       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
       CritLog ( << sdp.session().media().size());       
       assert(sdp.session().media().size() == 2);
       
       CritLog(<< "Marconi Test Ok");
    }

    {
       Data txt("v=0\r\n"
                "o=ff_AT_tye.idv.tw 170748954 170754822 IN IP4 202.5.224.96\r\n"
                "s=X-Lite\r\n"
                "c=IN IP4 202.5.224.96\r\n"
                "t=0 0\r\n"
                "m=audio 12000 RTP/AVP 98 101\r\n"
                "a=fmtp:101 0-15\r\n"
                "a=rtpmap:98 iLBC/8000\r\n"
                "a=rtpmap:101 telephone-event/8000\r\n");

       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
       assert(sdp.session().connection().getAddress() == "202.5.224.96");
       assert(sdp.session().media().front().port() == 12000);
       assert(sdp.session().media().front().getValues("fmtp").front() == "101 0-15");
       assert(sdp.session().media().front().getValues("rtpmap").front() == "98 iLBC/8000");
       assert(*++sdp.session().media().front().getValues("rtpmap").begin() == "101 telephone-event/8000");

       assert(sdp.session().media().front().codecs().front().getName() == "iLBC");

       CritLog(<< "Ok");
    }

    //exit(0);
    
    {
       Data txt("v=0\r\n"
                "o=CiscoSystemsSIP-GW-UserAgent 2087 3916 IN IP4 64.124.66.33\r\n"
                "s=SIP Call\r\n"
                "c=IN IP4 64.124.66.33\r\n"
                "t=0 0\r\n"
                "m=audio 12004 RTP/AVP 0 19\r\n"
                "c=IN IP4 64.124.66.33\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=rtpmap:19 CN/8000\r\n");
       
       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);
       assert(sdp.session().connection().getAddress() == "64.124.66.33");
       assert(sdp.session().media().front().port() == 12004);
    }
    
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
      SdpContents sdp(hfv, type);

      assert(sdp.session().media().front().codecs().size() == 5);
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
      SdpContents sdp(hfv, type);

      HeaderFieldValue hfv2(txt.data(), txt.size());
      SdpContents sdp2(hfv2, type);

      sdp.session();
      sdp2.session();

      Data sdpO = Data::from(sdp);
      sdp = sdp2;
      Data sdpO2 = Data::from(sdp);

      cerr << "!! " << sdp << endl;
      assert(sdpO == sdpO2);
    }
    
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
      SdpContents sdp(hfv, type);

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
      SdpContents sdp(hfv, type);
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
      SdpContents sdp(hfv, type);
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
   
#if 0 // .slg. removing this test case - attribute order is currently not guaranteed to be mainted
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

      //cout << encoded;
      //cout << shouldBeLike;
      assert(encoded == shouldBeLike);
      tassert_verify(4);
   }
   tassert_report();
#endif

   {
      Data txt("v=0\r\n"
               "o=ray.zibman 846934093 1 IN IP4 66.152.249.120\r\n"
               "s=phone-call\r\n"
               "c=IN IP4 66.152.249.120\r\n"
               "b=CT 1000\r\n" // should be CT:1000
               "t=0 0\r\n"
               "m=audio 12002 RTP/AVP 0 101\r\n"
               "a=rtpmap:0 PCMU/8000\r\n"
               "a=rtpmap:101 telephone-event/8000\r\n"
               "a=fmtp:101 0-16\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(hfv, type);

      try
      {
         assert(sdp.session().media().front().codecs().size() == 2);
         assert(false);
      }
      catch (ParseException& e)
      {
         // bad bandwidth
      }
   }

   {
      Data txt("v=0\r\n"
               "o=anonymous 1076575175 1076575175 IN IP4 192.168.1.100\r\n"
               "s=eConf 4.0\r\n"
               "i=eConf 4.0\r\n"
               "b=AS:256\r\n"
               "t=0 0\r\n"
               "m=audio 6000 RTP/AVP 102 104 9 4 0 8 98\r\n"
               "a=fmtp:98 0-15\r\n"
               "a=rtpmap:102 X-G72x1/16000\r\n"
               "a=rtpmap:104 X-G72x24/16000\r\n"
               "a=rtpmap:9 G722/8000\r\n"
               "a=rtpmap:4 G723/8000\r\n"
               "a=rtpmap:0 PCMU/8000\r\n"
               "a=rtpmap:8 PCMA/8000\r\n"
               "a=rtpmap:98 telephone-event/8000\r\n"
               "a=sendrecv\r\n"
               "m=video 6002 RTP/AVP 97 98 34 31\r\n"
               "b=AS:192\r\n"
               "a=fmtp:97 QCIF=1/MaxBR=1920/\r\n"
               "a=framerate:25.0\r\n"
               "a=fmtp:34 QCIF=1/MaxBR=1920\r\n"
               "a=fmtp:31 QCIF=1/MaxBR=1920\r\n"
               "a=rtpmap:97 H263-1998/90000\r\n"
               "a=rtpmap:98 MP4V-ES/90000\r\n"
               "a=rtpmap:34 H263/90000\r\n"
               "a=rtpmap:31 H261/90000\r\n"
               "a=sendrecv\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(hfv, type);

      assert(sdp.session().information() == "eConf 4.0");
      assert(sdp.session().media().size() == 2);
   }

#if 0   //.dcm. -- we don't validate, so this failure isn't something we are
        //planning to fix afaik
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
      SdpContents sdp(hfv, type);

      tassert(sdp.session().version() == 0);
      tassert(sdp.session().origin().user() == "alice");
      tassert(!sdp.session().media().empty());
      //this fails, but should probably not parse(t before c not in sdp)
      tassert(sdp.session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");
      tassert_verify(5);
   }
#endif


   {
      Data txt("v=0\r\n"
               "o=CiscoSystemsSIP-GW-UserAgent 4316 2064 IN IP4 65.39.205.114\r\n"
               "s=SIP Call\r\n"
               "c=IN IP4 65.39.205.114\r\n"
               "t=0 0\r\n"
               "m=audio 36928 RTP/AVP 0\r\n"
               "c=IN IP4 65.39.205.114\r\n"
               "a=rtpmap:0 PCMU/8000\r\n"
               "m=video 36924 RTP/AVP\r\n"
               "c=IN IP4 65.39.205.114\r\n");
      
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(hfv, type);

      assert(sdp.session().media().size() == 2);
   }

   {
      Data txt("v=0\r\n"
               "o=test 846934093 1 IN IP4 10.10.10.10\r\n"
               "s=SIP Call\r\n"
               "c=IN IP4 10.10.10.10\r\n"
               "t=0 0\r\n"
               "m=audio 12002 RTP/AVP 9 101\r\n"
               "a=rtpmap:101 telephone-event/8000\r\n"
               "a=fmtp:9 annexb=no\r\n"
               "a=fmtp:101 0-16\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents sdp(hfv, type);

      assert(sdp.session().media().front().codecs().size() == 2);
      assert(sdp.session().media().front().codecs().front().payloadType() == 9);
      assert(sdp.session().media().front().codecs().front().parameters() == "annexb=no");
      assert(sdp.session().media().front().codecs().back().payloadType() == 101);
      assert(sdp.session().media().front().codecs().back().parameters() == "0-16");
   }

   {
       Data txt("v=0\r\n"
                "o=Dialogic_IPCCLib 147345984 147345984 IN IP4 58.185.204.251\r\n"
                "s=Dialogic_SIP_CCLIB\r\n"
                "i=session information\r\n"
                "c=IN IP4 58.185.204.251\r\n"
                "t=0 0\r\n"
                "m=audio 49172 RTP/AVP 0 101\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=fmtp\r\n"
                "a=rtpmap:101 telephone-event/8000\r\n"
                "m=video 57364 RTP/AVP 34\r\n"
                "b=AS:42\r\n"
                "a=rtpmap:34 H263/90\r\n");
       

       HeaderFieldValue hfv(txt.data(), txt.size());
       Mime type("application", "sdp");
       SdpContents sdp(hfv, type);

       assert(sdp.session().media().size() == 2);
       assert(sdp.session().media().front().getValues("fmtp").front() == "");
       assert(sdp.session().media().front().codecs().size() == 2); // 0 and 101
       assert(sdp.session().media().front().codecs().front().parameters().size() == 0);
       assert(sdp.session().media().front().codecs().back().parameters().size() == 0);       
       
       CritLog(<< "Received bad Dialogic fmtp line Ok");
    }

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
