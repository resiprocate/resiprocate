#include <rutil/Logger.hxx>
#include <media/sdpcontainer/SdpHelper.hxx>

#include <iostream>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;

   if (argc > 1)
   {
      switch (*argv[1])
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
   CritLog(<< "SdpHelper Test Driver Starting");

   // Simple SDP
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
      SdpContents stackSdp(hfv, type);
      std::auto_ptr<sdpcontainer::Sdp> sdp(SdpHelper::createSdpFromResipSdp(stackSdp));

      cout << *sdp << endl;

      assert(sdp->getSdpVersion() == 0);

      assert(sdp->getOriginatorUserName() == "VGW");
      assert(sdp->getOriginatorSessionId() == 1251901012);
      assert(sdp->getOriginatorSessionVersion() == 1251901012);
      assert(sdp->getOriginatorNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(sdp->getOriginatorAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(sdp->getOriginatorUnicastAddress() == "10.1.83.143");

      assert(sdp->getSessionName() == "VGW");

      assert(sdp->getTimes().size() == 1);
      assert(sdp->getTimes().front().getStartTime() == 0);
      assert(sdp->getTimes().front().getStopTime() == 0);

      assert(sdp->getMediaLines().size() == 1);
      sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().front();
      assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
      assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP);
      assert(mediaLine.getConnections().size() == 1);
      assert(mediaLine.getConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(mediaLine.getConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(mediaLine.getConnections().front().getAddress() == "10.1.83.143");
      assert(mediaLine.getConnections().front().getPort() == 45894);
      assert(mediaLine.getRtcpConnections().size() == 1);
      assert(mediaLine.getRtcpConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(mediaLine.getRtcpConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(mediaLine.getRtcpConnections().front().getAddress() == "10.1.83.143");
      assert(mediaLine.getRtcpConnections().front().getPort() == 45895);

      assert(mediaLine.getCodecs().size() == 1);
      assert(mediaLine.getCodecs().front().getPayloadType() == 103);
      assert(mediaLine.getCodecs().front().getMimeSubtype() == "ISAC");
      assert(mediaLine.getCodecs().front().getRate() == 16000);
      assert(mediaLine.getCodecs().front().getFormatParameters() == "mode=30, type=fixed, bitrate=32000");
      assert(mediaLine.getCodecs().front().getNumChannels() == 1);

      assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
   }

   // RFC5939 (Capability Negotiation) Example 4.1
   // Note: The Sdp class doesn't currently support the rtcp-db attribute, so it will ignored
   {
      Data txt("v=0\r\n"
         "o=- 25678 753849 IN IP4 192.0.2.1\r\n"
         "s=\r\n"
         "c=IN IP4 192.0.2.1\r\n"
         "t=0 0\r\n"
         "m=audio 53456 RTP/AVP 0 18\r\n"
         "a=tcap:1 RTP/SAVPF RTP/SAVP RTP/AVPF\r\n"
         "a=acap:1 crypto:1 AES_CM_128_HMAC_SHA1_80 inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz|2^20|1:4 FEC_ORDER=FEC_SRTP\r\n"
         "a=acap:2 rtcp-fb:0 nack\r\n"
         "a=pcfg:1 t=1 a=1,[2]\r\n"
         "a=pcfg:2 t=2 a=1\r\n"
         "a=pcfg:3 t=3 a=[2]\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents stackSdp(hfv, type);
      std::auto_ptr<sdpcontainer::Sdp> sdp(SdpHelper::createSdpFromResipSdp(stackSdp));

      cout << *sdp << endl;

      assert(sdp->getSdpVersion() == 0);

      assert(sdp->getOriginatorUserName() == "-");
      assert(sdp->getOriginatorSessionId() == 25678);
      assert(sdp->getOriginatorSessionVersion() == 753849);
      assert(sdp->getOriginatorNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(sdp->getOriginatorAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(sdp->getOriginatorUnicastAddress() == "192.0.2.1");

      assert(sdp->getSessionName() == "");

      assert(sdp->getTimes().size() == 1);
      assert(sdp->getTimes().front().getStartTime() == 0);
      assert(sdp->getTimes().front().getStopTime() == 0);

      assert(sdp->getMediaLines().size() == 1);
      sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().front();
      assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
      assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP);
      assert(mediaLine.getConnections().size() == 1);
      assert(mediaLine.getConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(mediaLine.getConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(mediaLine.getConnections().front().getAddress() == "192.0.2.1");
      assert(mediaLine.getConnections().front().getPort() == 53456);
      assert(mediaLine.getRtcpConnections().size() == 1);
      assert(mediaLine.getRtcpConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(mediaLine.getRtcpConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(mediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
      assert(mediaLine.getRtcpConnections().front().getPort() == 53457);
      assert(mediaLine.getCryptos().size() == 0);

      assert(mediaLine.getCodecs().size() == 2);
      assert(mediaLine.getCodecs().front().getPayloadType() == 0);
      assert(mediaLine.getCodecs().front().getMimeSubtype() == "PCMU");
      assert(mediaLine.getCodecs().front().getRate() == 8000);
      assert(mediaLine.getCodecs().front().getNumChannels() == 1);
      assert(mediaLine.getCodecs().back().getPayloadType() == 18);
      assert(mediaLine.getCodecs().back().getMimeSubtype() == "G729");
      assert(mediaLine.getCodecs().back().getRate() == 8000);
      assert(mediaLine.getCodecs().back().getNumChannels() == 1);

      assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

      assert(mediaLine.getPotentialMediaViews().size() == 3);
      assert(mediaLine.getPotentialMediaViews().front().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF);
      assert(mediaLine.getPotentialMediaViews().front().getPotentialMediaViewString() == "1 t=1 a=1,[2]");
      assert((++mediaLine.getPotentialMediaViews().begin())->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
      assert((++mediaLine.getPotentialMediaViews().begin())->getPotentialMediaViewString() == "2 t=2 a=1");
      assert(mediaLine.getPotentialMediaViews().back().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVPF);
      assert(mediaLine.getPotentialMediaViews().back().getPotentialMediaViewString() == "3 t=3 a=[2]");

      const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().front();

      assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
      assert(potentialMediaLine.getConnections().size() == 1);
      assert(potentialMediaLine.getConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(potentialMediaLine.getConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
      assert(potentialMediaLine.getConnections().front().getPort() == 53456);
      assert(potentialMediaLine.getRtcpConnections().size() == 1);
      assert(potentialMediaLine.getRtcpConnections().front().getNetType() == sdpcontainer::Sdp::NET_TYPE_IN);
      assert(potentialMediaLine.getRtcpConnections().front().getAddressType() == sdpcontainer::Sdp::ADDRESS_TYPE_IP4);
      assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
      assert(potentialMediaLine.getRtcpConnections().front().getPort() == 53457);
      assert(potentialMediaLine.getCryptos().size() == 1);
      assert(potentialMediaLine.getCryptos().front().getTag() == 1);
      assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80);
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz");
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
      assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 4);
      // Note: FEC_ORDER=FEC_SRTP is currently not parsed by sdpcontainer

      assert(potentialMediaLine.getCodecs().size() == 2);
      assert(potentialMediaLine.getCodecs().front().getPayloadType() == 0);
      assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "PCMU");
      assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
      assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);
      assert(potentialMediaLine.getCodecs().back().getPayloadType() == 18);
      assert(potentialMediaLine.getCodecs().back().getMimeSubtype() == "G729");
      assert(potentialMediaLine.getCodecs().back().getRate() == 8000);
      assert(potentialMediaLine.getCodecs().back().getNumChannels() == 1);

      assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
   }

   // RFC5939 (Capability Negotiation) Example 4.2
   // Note: The Sdp class doesn't currently support the rtcp-db attribute, so it will ignored
   {
      Data txt("v=0\r\n"
         "o=- 25678 753849 IN IP4 192.0.2.1\r\n"
         "s=\r\n"
         "c=IN IP4 192.0.2.1\r\n"  // c= and t= lines are reversed in RFC
         "t=0 0\r\n"
         "a=acap:1 setup:actpass\r\n"
         "a=acap:2 fingerprint: SHA-1 4A:AD:B9:B1:3F:82:18:3B:54:02:12:DF:3E:5D:49:6B:19:E5:7C:AB\r\n"
         "a=tcap:1 UDP/TLS/RTP/SAVP RTP/SAVP\r\n"
         "m=audio 59000 RTP/AVP 98\r\n"
         "a=rtpmap:98 AMR/8000\r\n"
         "a=acap:3 crypto:1 AES_CM_128_HMAC_SHA1_32 inline:NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj|2^20|1:32\r\n"
         "a=pcfg:1 t=1 a=1,2\r\n"
         "a=pcfg:2 t=2 a=3\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents stackSdp(hfv, type);
      std::auto_ptr<sdpcontainer::Sdp> sdp(SdpHelper::createSdpFromResipSdp(stackSdp));

      cout << *sdp << endl;

      // Note:  purposing skipping common sdpcontainer parsing tests that already passed above

      assert(sdp->getMediaLines().size() == 1);
      sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().front();
      assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
      assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP);
      assert(mediaLine.getConnections().size() == 1);
      assert(mediaLine.getConnections().front().getAddress() == "192.0.2.1");
      assert(mediaLine.getConnections().front().getPort() == 59000);
      assert(mediaLine.getRtcpConnections().size() == 1);
      assert(mediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
      assert(mediaLine.getRtcpConnections().front().getPort() == 59001);
      assert(mediaLine.getCryptos().size() == 0);

      assert(mediaLine.getCodecs().size() == 1);
      assert(mediaLine.getCodecs().front().getPayloadType() == 98);
      assert(mediaLine.getCodecs().front().getMimeSubtype() == "AMR");
      assert(mediaLine.getCodecs().front().getRate() == 8000);
      assert(mediaLine.getCodecs().front().getNumChannels() == 1);

      assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

      assert(mediaLine.getPotentialMediaViews().size() == 2);
      assert(mediaLine.getPotentialMediaViews().front().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP);
      assert(mediaLine.getPotentialMediaViews().front().getPotentialMediaViewString() == "1 t=1 a=1,2");
      assert(mediaLine.getPotentialMediaViews().back().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
      assert(mediaLine.getPotentialMediaViews().back().getPotentialMediaViewString() == "2 t=2 a=3");

      {  // Validate potential media view 1
         const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().front();

         assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
         assert(potentialMediaLine.getConnections().size() == 1);
         assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
         assert(potentialMediaLine.getConnections().front().getPort() == 59000);
         assert(potentialMediaLine.getRtcpConnections().size() == 1);
         assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
         assert(potentialMediaLine.getRtcpConnections().front().getPort() == 59001);
         assert(potentialMediaLine.getCryptos().size() == 0);
         assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_ACTPASS);
         assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_SHA_1);
         assert(potentialMediaLine.getFingerPrint() == "4A:AD:B9:B1:3F:82:18:3B:54:02:12:DF:3E:5D:49:6B:19:E5:7C:AB");

         assert(potentialMediaLine.getCodecs().size() == 1);
         assert(potentialMediaLine.getCodecs().front().getPayloadType() == 98);
         assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "AMR");
         assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
         assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

         assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
      }

      {  // Validate potential media view 2
         const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().back();

         assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
         assert(potentialMediaLine.getConnections().size() == 1);
         assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
         assert(potentialMediaLine.getConnections().front().getPort() == 59000);
         assert(potentialMediaLine.getRtcpConnections().size() == 1);
         assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
         assert(potentialMediaLine.getRtcpConnections().front().getPort() == 59001);
         assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE);
         assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE);
         assert(potentialMediaLine.getCryptos().size() == 1);
         assert(potentialMediaLine.getCryptos().front().getTag() == 1);
         assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32);
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj");
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
         assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);

         assert(potentialMediaLine.getCodecs().size() == 1);
         assert(potentialMediaLine.getCodecs().front().getPayloadType() == 98);
         assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "AMR");
         assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
         assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

         assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
      }
   }

   // RFC5939 (Capability Negotiation) Example 4.3
   // Note: The Sdp class doesn't currently support the rtcp-db attribute, so it will ignored
   {
      Data txt("v=0\r\n"
         "o=- 25678 753849 IN IP4 192.0.2.1\r\n"
         "s=\r\n"
         "c=IN IP4 192.0.2.1\r\n"   // c= and t= lines are reversed in RFC
         "t=0 0\r\n"
         "a=acap:1 key-mgmt:mikey AQAFgM0XflABAAAAAAAAAAAAAAsAyO...\r\n"
         "a=tcap:1 RTP/SAVPF RTP/SAVP RTP/AVPF\r\n"
         "m=audio 59000 RTP/AVP 98\r\n"
         "a=rtpmap:98 AMR/8000\r\n"
         "a=acap:2 crypto:1 AES_CM_128_HMAC_SHA1_32 inline:NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj|2^20|1:32\r\n"
         "a=pcfg:1 t=2 a=1|2\r\n"
         "m=video 52000 RTP/AVP 31\r\n"
         "a=rtpmap:31 H261/90000\r\n"
         "a=acap:3 crypto:1 AES_CM_128_HMAC_SHA1_80 inline:d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj|2^20|1:32\r\n"
         "a=acap:4 rtcp-fb:* nack\r\n"
         "a=pcfg:1 t=1 a=1,4|3,4\r\n"
         "a=pcfg:2 t=2 a=1|3\r\n"
         "a=pcfg:3 t=3 a=4\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents stackSdp(hfv, type);
      std::auto_ptr<sdpcontainer::Sdp> sdp(SdpHelper::createSdpFromResipSdp(stackSdp));

      cout << *sdp << endl;

      // Note:  purposing skipping common sdpcontainer parsing tests that already passed above

      assert(sdp->getMediaLines().size() == 2);
      {  // Validate media line 1
         sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().front();
         assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
         assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP);
         assert(mediaLine.getCryptos().size() == 0);
         assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

         assert(mediaLine.getPotentialMediaViews().size() == 2);
         assert(mediaLine.getPotentialMediaViews().front().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
         assert(mediaLine.getPotentialMediaViews().front().getPotentialMediaViewString() == "1 t=2 a=1");
         assert(mediaLine.getPotentialMediaViews().back().getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
         assert(mediaLine.getPotentialMediaViews().back().getPotentialMediaViewString() == "1 t=2 a=2");

         {  // Validate potential media view 1
            const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().front();

            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 59000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 59001);
            assert(potentialMediaLine.getCryptos().size() == 0);
            assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_MIKEY);
            assert(potentialMediaLine.getKeyManagementData() == "AQAFgM0XflABAAAAAAAAAAAAAAsAyO...");

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 98);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "AMR");
            assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
         {  // Validate potential media view 2
            const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().back();

            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 59000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 59001);
            assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_NONE);
            assert(potentialMediaLine.getCryptos().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getTag() == 1);
            assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj");
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 98);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "AMR");
            assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
      }

      {  // Validate media line 2
         sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().back();
         assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
         assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP);
         assert(mediaLine.getCryptos().size() == 0);
         assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

         assert(mediaLine.getPotentialMediaViews().size() == 5);

         auto itPotentialMediaView = mediaLine.getPotentialMediaViews().begin();
         {  // Validate potential media view 1
            const sdpcontainer::SdpMediaLine& potentialMediaLine = *itPotentialMediaView;

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF);
            assert(potentialMediaLine.getPotentialMediaViewString() == "1 t=1 a=1,4");
            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 52000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 52001);
            assert(potentialMediaLine.getCryptos().size() == 0);
            assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_MIKEY);
            assert(potentialMediaLine.getKeyManagementData() == "AQAFgM0XflABAAAAAAAAAAAAAAsAyO...");

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 31);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "H261");
            assert(potentialMediaLine.getCodecs().front().getRate() == 90000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
         {  // Validate potential media view 2
            const sdpcontainer::SdpMediaLine& potentialMediaLine = *(++itPotentialMediaView);

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF);
            assert(potentialMediaLine.getPotentialMediaViewString() == "1 t=1 a=3,4");
            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 52000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 52001);
            assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE);
            assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE);
            assert(potentialMediaLine.getCryptos().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getTag() == 1);
            assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj");
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 31);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "H261");
            assert(potentialMediaLine.getCodecs().front().getRate() == 90000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
         {  // Validate potential media view 3
            const sdpcontainer::SdpMediaLine& potentialMediaLine = *(++itPotentialMediaView);

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
            assert(potentialMediaLine.getPotentialMediaViewString() == "2 t=2 a=1");
            assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE);
            assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE);
            assert(potentialMediaLine.getCryptos().size() == 0);
            assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_MIKEY);
            assert(potentialMediaLine.getKeyManagementData() == "AQAFgM0XflABAAAAAAAAAAAAAAsAyO...");
         }
         {  // Validate potential media view 4
            const sdpcontainer::SdpMediaLine& potentialMediaLine = *(++itPotentialMediaView);

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
            assert(potentialMediaLine.getPotentialMediaViewString() == "2 t=2 a=3");
            assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE);
            assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE);
            assert(potentialMediaLine.getCryptos().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getTag() == 1);
            assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj");
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);
         }
         {  // Validate potential media view 5
            const sdpcontainer::SdpMediaLine& potentialMediaLine = *(++itPotentialMediaView);

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVPF);
            assert(potentialMediaLine.getPotentialMediaViewString() == "3 t=3 a=4");
            assert(potentialMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE);
            assert(potentialMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE);
            assert(potentialMediaLine.getCryptos().size() == 0);
            // sdpcontainer doesn't currently support rtcp-fb attributes, so we won't validate that here;
         }
      }
   }

   // RFC5939 (Capability Negotiation) Example 4.4
   // Note:  Since we don't handle attribute deletion in potential media views yet, the potential media view in this
   //        example won't work, since it will return the session level a=key=mgmt attribute when it shouldn't, see
   //        TODO notes below.
   {
      Data txt("v=0\r\n"
         "o=- 25678 753849 IN IP4 192.0.2.1\r\n"
         "s=\r\n"
         "c=IN IP4 192.0.2.1\r\n"  // c= and t= lines are reversed in RFC
         "t=0 0\r\n"
         "a=key-mgmt:mikey AQAFgM0XflABAAAAAAAAAAAAAAsAyO...\r\n"
         "m=audio 59000 RTP/SAVP 98\r\n"
         "a=rtpmap:98 AMR/8000\r\n"
         "a=acap:1 crypto:1 AES_CM_128_HMAC_SHA1_32 inline:NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj|2^20|1:32\r\n"
         "a=pcfg:1 a=-s:1\r\n"
         "m=video 52000 RTP/SAVP 31\r\n"
         "a=rtpmap:31 H261/90000\r\n"
         "a=acap:2 crypto:1 AES_CM_128_HMAC_SHA1_80 inline:d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj|2^20|1:32\r\n"
         "a=pcfg:1 a=-s:2\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "sdp");
      SdpContents stackSdp(hfv, type);
      std::auto_ptr<sdpcontainer::Sdp> sdp(SdpHelper::createSdpFromResipSdp(stackSdp));

      cout << *sdp << endl;

      // Note:  purposing skipping common sdpcontainer parsing tests that already passed above

      assert(sdp->getMediaLines().size() == 2);
      {  // Validate media line 1
         sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().front();
         assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
         assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
         assert(mediaLine.getCryptos().size() == 0);
         assert(mediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_MIKEY);
         assert(mediaLine.getKeyManagementData() == "AQAFgM0XflABAAAAAAAAAAAAAAsAyO...");
         assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

         assert(mediaLine.getPotentialMediaViews().size() == 1);

         {  // Validate potential media view 1
            const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().front();

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
            assert(potentialMediaLine.getPotentialMediaViewString() == "1 a=-s:1");  // indicates all session level attributes are to be deleted
            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 59000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 59001);
            // TODO - deletes not implemented, so this will fail
            //assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_NONE);
            assert(potentialMediaLine.getCryptos().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getTag() == 1);
            assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "NzB4d1BINUAvLEw6UzF3WSJ+PSdFcGdUJShpX1Zj");
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 98);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "AMR");
            assert(potentialMediaLine.getCodecs().front().getRate() == 8000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
      }

      {  // Validate media line 2
         sdpcontainer::SdpMediaLine& mediaLine = *sdp->getMediaLines().back();
         assert(mediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
         assert(mediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
         assert(mediaLine.getCryptos().size() == 0);
         assert(mediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_MIKEY);
         assert(mediaLine.getKeyManagementData() == "AQAFgM0XflABAAAAAAAAAAAAAAsAyO...");
         assert(mediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);

         assert(mediaLine.getPotentialMediaViews().size() == 1);

         {  // Validate potential media view 1
            const sdpcontainer::SdpMediaLine& potentialMediaLine = mediaLine.getPotentialMediaViews().front();

            assert(potentialMediaLine.getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP);
            assert(potentialMediaLine.getPotentialMediaViewString() == "1 a=-s:2");
            assert(potentialMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            assert(potentialMediaLine.getConnections().size() == 1);
            assert(potentialMediaLine.getConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getConnections().front().getPort() == 52000);
            assert(potentialMediaLine.getRtcpConnections().size() == 1);
            assert(potentialMediaLine.getRtcpConnections().front().getAddress() == "192.0.2.1");
            assert(potentialMediaLine.getRtcpConnections().front().getPort() == 52001);
            // TODO - deletes not implemented, so this will fail
            //assert(potentialMediaLine.getKeyManagementProtocol() == sdpcontainer::SdpMediaLine::KEYMANAGEMENT_PROTOCOL_NONE);
            assert(potentialMediaLine.getCryptos().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getTag() == 1);
            assert(potentialMediaLine.getCryptos().front().getSuite() == sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().size() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyMethod() == sdpcontainer::SdpMediaLine::CRYPTO_KEY_METHOD_INLINE);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getKeyValue() == "d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj");
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpLifetime() == 1048576); // 2^20
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiValue() == 1);
            assert(potentialMediaLine.getCryptos().front().getCryptoKeyParams().front().getSrtpMkiLength() == 32);

            assert(potentialMediaLine.getCodecs().size() == 1);
            assert(potentialMediaLine.getCodecs().front().getPayloadType() == 31);
            assert(potentialMediaLine.getCodecs().front().getMimeSubtype() == "H261");
            assert(potentialMediaLine.getCodecs().front().getRate() == 90000);
            assert(potentialMediaLine.getCodecs().front().getNumChannels() == 1);

            assert(potentialMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV);
         }
      }
   }

   return 0;
}

/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. https://www.sipspectrum.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
