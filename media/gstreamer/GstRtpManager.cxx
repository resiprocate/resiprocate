#include "GstRtpManager.hxx"

#include <regex>

#include "GStreamerUtils.hxx"

#ifdef USE_GSTREAMER

using namespace resipgst;
using namespace resip;
using namespace Glib;
using namespace Gst;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::GstSubsystem::GSTREAMER


/*
 * Initiating a call:
 * - reserve ports
 * - create local UDP sockets
 * - generate SDP offer
 * - receive SDP answer
 * - intersect offer and answer
 * - create decoders
 * - create encoders
 *
 * Receiving a call:
 * - receive SDP offer
 * - reserve ports
 * - create local UDP sockets
 * - generate SDP answer (intersects with offer)
 * - create decoders
 * - create encoders
 */


GstRtpManager::GstRtpManager(const resip::Data& localAddress)
 : mLocalAddress(localAddress),
   mPortManager()
{
   initCodecConfigMap();
}

GstRtpManager::~GstRtpManager()
{
}

void
GstRtpManager::initCodecConfigMap()
{
   /**
    * Audio codecs
    */

   mCodecConfigMap.insert({"PCMU", make_shared<CodecConfig>("PCMU", "mulawdec", "mulawenc", "", "rtppcmudepay", "rtppcmupay", "", 8000, "")});
   mCodecConfigMap.insert({"PCMA", make_shared<CodecConfig>("PCMA", "alawdec", "alawenc", "", "rtppcmadepay", "rtppcmapay", "", 8000, "")});

   // FIXME - OPUS needs 2 channels?
   mCodecConfigMap.insert({"OPUS", make_shared<CodecConfig>("OPUS", "opusdec", "opusenc", "", "rtpopusdepay", "rtpopuspay", "", 48000, "")});

   /**
    * Video codecs
    */

   // Obtain a list of all H.264 plugins currently installed with
   // the command:
   //
   //   gst-inspect-1.0  | grep 264

   // OpenMAX IL https://www.phoronix.com/scan.php?page=news_item&px=Libav-OMX-H264-MPEG4
   // https://www.khronos.org/openmaxil
   // VA-API is more advanced
   // h264omx
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "avdec_h264", "avenc_h264_omx", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // not working
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // Decode: libav
   // Encode: x264 (GPL 2) https://www.videolan.org/developers/x264.html
   //    (x264 doesn't provide a decoder)
   // h264avx
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "avdec_h264", "x264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // works
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // need to recompile the package gstreamer1.0-plugins-bad selecting the openh264 config option
   // h264o
   mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "openh264dec", "openh264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // works for 30 seconds
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // VA-API for AMD and Intel GPU hardware decoding / encoding
   // improves quality while using less CPU and giving real-time performance
   // sudo apt install gstreamer1.0-vaapi
   // h264v
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "vaapih264dec", "vaapih264enc", "constrained-baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // garbled video stream
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // mix and match decoder and encoder
   // h264m-av-open
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "avdec_h264", "openh264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // works
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});
   // h264m-vaapi-x264
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "vaapih264dec", "x264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // works for a few seconds then replays
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});
   // h264m-open-x264
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "openh264dec", "x264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // not working
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});
   // h264m-vaapi-open
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "vaapih264dec", "openh264enc", "baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // works
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});
   // h264m-av-vaapi
   //mCodecConfigMap.insert({"H264", make_shared<CodecConfig>("H264", "avdec_h264", "vaapih264enc", "constrained-baseline", "rtph264depay", "rtph264pay", "h264parse", 90000, // not working
   //   "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // VP8 with the open source reference implementation
   // vp8
   mCodecConfigMap.insert({"VP8", make_shared<CodecConfig>("VP8", "vp8dec", "vp8enc", "", "rtpvp8depay", "rtpvp8pay", "vp8parse", 90000, "profile-level-id=HiP")});
}

GstRtpSession::GstRtpSession(GstRtpManager& rTPManager, bool webRTC)
 : mRTPManager(rTPManager),
   mWebRTC(webRTC)
{
   mMediaBin = Bin::create();
}

GstRtpSession::~GstRtpSession()
{

}

const Data&
GstRtpSession::getLocalAddress() const
{
   return mRTPManager.getLocalAddress();
}

unsigned int
GstRtpSession::allocatePort()
{
   // FIXME - must free the port when finished
   return mRTPManager.getPortManager().allocateRTPPort();
}

std::shared_ptr<SdpContents>
GstRtpSession::buildOffer(bool audio, bool video)
{
   std::shared_ptr<SdpContents> offer;

   unsigned int audioPort = allocatePort();
   unsigned int videoPort = allocatePort();

   const Data& localAddress = getLocalAddress();
   const CodecConfigMap& codecConfigs = mRTPManager.getCodecConfigMap();
   const CodecConfig& cAudio = *codecConfigs.at("PCMA");
   unsigned int ptAudio = 8;
   const CodecConfig& cVideo = *codecConfigs.at("H264");
   unsigned int ptVideo = 97;
   // FIXME - free txt, IPv6, timestamps, other static stuff...
   Data* txt = new Data("v=0\r\n"
                           "o=- 3838180699 3838180699 IN IP4 " + localAddress + "\r\n"
                           "s=reSIProcate\r\n"
                           "c=IN IP4 " + localAddress + "\r\n"
                           "t=0 0\r\n"
                           "a=ice-pwd:da9801364d7cd7d3a87f7f2f\r\n"
                           "a=ice-ufrag:91f7ed7e\r\n"
                           "a=rtcp-xr:rcvr-rtt=all:10000 stat-summary=loss,dup,jitt,TTL voip-metrics\r\n"
                           "m=audio " + Data(audioPort) + " RTP/AVP " + Data(ptAudio) + "\r\n"
                           "a=sendrecv\r\n"
                           "a=rtcp:" + Data(audioPort+1) + "\r\n"
                           "a=rtpmap:" + Data(ptAudio) + " " + cAudio.mName + "/" + Data(cAudio.mRate) + "\r\n"
                           //"a=ssrc:2337389544 cname:user269660271@host-9999cdcf\r\n"
                           "m=video " + Data(videoPort) + " RTP/AVP " + Data(ptVideo) + "\r\n"
                           "a=sendrecv\r\n"
                           "a=rtcp:" + Data(videoPort+1) + "\r\n"
                           "a=rtpmap:" + Data(ptVideo) + " " + cVideo.mName + "/" + Data(cVideo.mRate) + "\r\n"
                           "a=fmtp:" + Data(ptVideo) + " " + cVideo.mFmtp + "\r\n"
                           //"a=ssrc:2005192486 cname:user269660271@host-9999cdcf\r\n"
                           );

   HeaderFieldValue *hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
   Mime type("application", "sdp");
   mLocal = std::make_shared<SdpContents>(SdpContents(*hfv, type));

   return mLocal;
}

// disable the stream by setting port to 0 as per RFC 3264 s6
// FIXME: move this logic to SdpContents
void
rejectStream(ostream& o, const SdpContents::Session::Medium& m)
{
   o << "m=" << m.name() << " 0 " << m.protocol() << " ";
   if(!m.getFormats().empty())
   {
      o << m.getFormats().front();
   }
   else if(!m.codecs().empty())
   {
      o << m.codecs().front().payloadType();
   }
   else
   {
      o << "*";
      // o << "0";  // an alternative
   }
   o << "\r\n";
}

std::shared_ptr<SdpContents>
GstRtpSession::buildAnswer(std::shared_ptr<resip::SdpContents> remoteOffer)
{
   Data* txt = new Data();
   DataStream stream(*txt);

   const Data& localAddress = getLocalAddress();
   // FIXME - free txt, IPv6, timestamps
   stream << "v=0\r\n"
            "o=- 3838180699 3838180699 IN IP4 " + localAddress + "\r\n"
            "s=reSIProcate\r\n"
            "c=IN IP4 " + localAddress + "\r\n"
            "t=0 0\r\n"
            "a=rtcp-xr:rcvr-rtt=all:10000 stat-summary=loss,dup,jitt,TTL voip-metrics\r\n";

   for(auto& m : remoteOffer->session().media())
   {
      if(m.name() == "audio" || m.name() == "video")
      {
         int pt = -1;
         const CodecConfigMap& codecConfigs = mRTPManager.getCodecConfigMap();
         CodecConfigMap::const_iterator it = codecConfigs.end();
         for(auto& c : m.codecs())
         {
            it = codecConfigs.find(c.getName());  // FIXME - check all codec properties are compatible
            if(it != codecConfigs.end())
            {
               pt = c.payloadType();
               DebugLog(<<"found supported codec, using peer's payload type " << pt);
               break;
            }
         }
         if(it != codecConfigs.end())
         {
            const CodecConfig& cc = *it->second;
            unsigned int localPort = allocatePort();
            stream << "m=" << m.name() << " " << localPort << " RTP/AVP " << pt << "\r\n"
                   << "a=sendrecv\r\n";
            stream << "a=rtpmap:" << pt << " " << cc.mName << "/" << cc.mRate << "\r\n";
            if(m.name() == "video")
            {
               stream << "a=fmtp:" << pt << " " << cc.mFmtp << "\r\n";
            }
         }
         else
         {
            DebugLog(<<"didn't find supported codec");
            rejectStream(stream, m);
         }
      }
      else
      {
         DebugLog(<<"unsupported media type");
         rejectStream(stream, m);
      }
   }

   stream.flush();
   HeaderFieldValue *hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
   Mime type("application", "sdp");
   mLocal = std::make_shared<SdpContents>(SdpContents(*hfv, type));

   mRemote = remoteOffer;
   return mLocal;
}

void
GstRtpSession::processAnswer(std::shared_ptr<resip::SdpContents> remoteAnswer)
{
   // FIXME
   ErrLog(<<"incomplete, answer not processed");
   resip_assert(0);

   mRemote = remoteAnswer;

   // doOfferAnswerIntersect
}

Glib::RefPtr<Gst::Caps>
GstRtpSession::getCaps(SdpContents::Session::Medium& m)
{
   SdpContents::Session::Codec& c = m.codecs().front();
   /*Data caps = "application/x-rtp,media=" + m.name()
            + ",encoding-name=" + c.getName()
            + ",clock-rate=" + Data(c.getRate())
            + ",payload=" + Data(c.payloadType());*/
   /*Data caps = "application/x-rtp,media=(string)" + m.name()
               + ",encoding-name=(string)" + c.getName()
               + ",clock-rate=(int)" + Data(c.getRate())
               + ",payload=(int)" + Data(c.payloadType());
   return Gst::Caps::create_from_string(caps.c_str());*/
   return Gst::Caps::create_simple("application/x-rtp",
            "media", m.name().c_str(),
            "encoding-name", c.getName().c_str(),
            "clock-rate", c.getRate(),
            "payload", c.payloadType());
}

Glib::RefPtr<Gst::Caps>
GstRtpSession::getCaps(const resip::Data& mediumName)
{
   std::list<std::reference_wrapper<SdpContents::Session::Medium>> media =
            mLocal->session().getMediaByType(mediumName);
   if(media.size() == 0)
   {
      WarningLog(<<"no medium found: " << mediumName);
      return RefPtr<Gst::Caps>();
   }
   SdpContents::Session::Medium& m = media.front().get();
   return getCaps(m);
}

RefPtr<Bin>
GstRtpSession::createOutgoingPipeline(const RefPtr<Caps> caps)
{
   DebugLog(<<"creating outgoing bin for caps: " << caps->to_string());
   RefPtr<Bin> encodeBin = Bin::create();

   RefPtr<Queue> queueIn = Queue::create();
   encodeBin->add(queueIn);

   RefPtr<Pad> queueInSrc = queueIn->get_static_pad("src");

   string mediaType = caps->get_structure(0).get_name();
   StackLog(<<"mediaType: " << mediaType);
   string mediaName;
   if(caps->get_structure(0).has_field("media"))
   {
      caps->get_structure(0).get_field("media", mediaName);
   }
   StackLog(<<"mediaName: " << mediaName);
   string encodingName;
   if(caps->get_structure(0).has_field("encoding-name"))
   {
      caps->get_structure(0).get_field("encoding-name", encodingName);
   }
   StackLog(<<"encodingName: " << encodingName);
   Data payloaderName;
   RefPtr<Element> enc;
   RefPtr<Gst::Element> payloader;
   Glib::RefPtr<Gst::Caps> v_caps;
   if(regex_search(mediaType, regex("^video")) || mediaName == "video")
   {
      //GstCaps* __caps = gst_caps_from_string("video/x-raw");
      // profile stuff is only for Gst::EncodeBin
      //encodeBin->property_profile() =
      //GstEncodingVideoProfile* videoProfile =
      //         gst_encoding_video_profile_new(__caps, NULL, NULL, 1);
      //g_object_set(encodeBin->gobj(), "profile", videoProfile, NULL);
      if(encodingName == "VP8")
      {
         // https://gstreamer.freedesktop.org/documentation/vpx/vp8enc.html?gi-language=c
         // https://www.webmproject.org/docs/encoder-parameters/
         // The parameters have slightly different names in gstreamer:
         // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-vp8enc.html
         enc = Gst::ElementFactory::create_element("vp8enc");
         v_caps = Gst::Caps::create_simple("video/x-vp8",
                               "profile", "0");
         enc->property<gint64>("deadline", 0);
         //enc->property<bool>("rt", true);
         //enc->property<guint>("threads", 24); // FIXME magic number
         //enc->property<GstVP8EncTokenPartitions>("token-partitions", 3);  // FIXME magic number - requires 8 cores/threads
         enc->property<gint>("token-partitions", 3);  // FIXME magic number - requires 8 cores/threads
         enc->property<gint>("keyframe-max-dist", 2000);
         // FIXME properties on vp8enc
         payloader = Gst::ElementFactory::create_element("rtpvp8pay");
         payloader->set_property<gint32>("pt", 120); // FIXME magic number
      }
      else if(encodingName == "H264")
      {
         // https://gstreamer.freedesktop.org/documentation/openh264/openh264enc.html?gi-language=c
         enc = Gst::ElementFactory::create_element("openh264enc");
         v_caps = Gst::Caps::create_simple("video/x-h264",
                      "profile", "baseline");
         enc->property<guint>("bitrate", 90000); // low // FIXME magic number
         enc->property<gint64>("complexity", 0); // low
         enc->property<gint64>("rate-control", 1); // bitrate // FIXME magic number

         // https://gstreamer.freedesktop.org/documentation/vaapi/vaapih264enc.html?gi-language=c
         /*enc = Gst::ElementFactory::create_element("vaapih264enc");
         v_caps = Gst::Caps::create_simple("video/x-h264",
                               "profile", "constrained-baseline");
         enc->property<guint>("bitrate", 90); // low // FIXME magic number
         //enc->property<guint>("quality-factor", 26); // ICQ/QVBR bitrate control mode quality
         enc->property<guint>("quality-level", 10); // faster encode, lower quality
         enc->property<gint64>("rate-control", 2); // bitrate // FIXME magic number */

         // https://gstreamer.freedesktop.org/documentation/x264/index.html?gi-language=c
         /*enc = Gst::ElementFactory::create_element("x264enc");
         v_caps = Gst::Caps::create_simple("video/x-h264",
                      "profile", "constrained-baseline");
         enc->property<guint>("bitrate", 90); // low // FIXME magic number
         enc->property<gint64>("pass", 0); // CBR // FIXME magic number
         enc->property<gint64>("tune", 4); // zero latency // FIXME magic number */

         // FIXME properties on enc
         payloader = Gst::ElementFactory::create_element("rtph264pay");
         payloader->set_property<gint32>("pt", 97); // FIXME magic number
      }
      else
      {
         ErrLog(<<"unsupported encoding: " << encodingName);
         resip_assert(0);
      }
   }
   else if(regex_search(mediaType, regex("^audio")) || mediaName == "audio")
   {
      //GstCaps* __caps = gst_caps_from_string("audio/x-raw");
      // profile stuff is only for Gst::EncodeBin
      //encodeBin->property_profile() =
      //GstEncodingAudioProfile* audioProfile =
      //         gst_encoding_audio_profile_new(__caps, NULL, NULL, 1);
      //g_object_set(encodeBin->gobj(), "profile", audioProfile, NULL);
      if(encodingName == "OPUS")
      {
         // https://gstreamer.freedesktop.org/documentation/opus/opusenc.html?gi-language=c
         enc = Gst::ElementFactory::create_element("opusenc");
         uint32_t pt = 109; // FIXME magic number
         v_caps = Gst::Caps::create_simple("audio/x-opus",
                                        "payload", pt,
                                        "encoding-name", "OPUS",
                                        "rate", 48000,       // FIXME magic number
                                        "channels", 2);     // FIXME magic number
         enc->property<gint>("bitrate", 48000);
         payloader = Gst::ElementFactory::create_element("rtpopuspay");
         payloader->set_property<gint32>("pt", pt);
      }
      else if(encodingName == "PCMA")
      {
         // https://gstreamer.freedesktop.org/documentation/alaw/alawenc.html?gi-language=c
         enc = Gst::ElementFactory::create_element("alawenc");
         v_caps = Gst::Caps::create_simple("audio/x-alaw",
                                        "rate", 8000,       // FIXME magic number
                                        "channels", 1);     // FIXME magic number
         payloader = Gst::ElementFactory::create_element("rtppcmapay");
         payloader->set_property<gint32>("pt", 8); // FIXME magic number
      }
      else
      {
         CritLog(<<"unsupported encoding: " << encodingName);
         resip_assert(0);
      }
   }
   else
   {
      ErrLog(<<"unhandled mediaType");
      resip_assert(0);
   }

   encodeBin->add(enc);
   encodeBin->add(payloader);
   RefPtr<Gst::Queue> queue = Gst::Queue::create("queueOut");
   encodeBin->add(queue);

   queueInSrc->link(enc->get_static_pad("sink"));

   if(!v_caps)
   {
      enc->link(payloader);
   }
   else
   {
      enc->link(payloader, v_caps);
   }
   payloader->link(queue, caps);
   RefPtr<Pad> queueSrc = queue->get_static_pad("src");

   RefPtr<Pad> sink = GhostPad::create(queueIn->get_static_pad("sink"), "sink");
   encodeBin->add_pad(sink);
   RefPtr<Pad> src = GhostPad::create(queueSrc, "src");
   encodeBin->add_pad(src);

   return encodeBin;
}

std::string
makePadName(const std::string& prefix, unsigned int streamId)
{
   ostringstream padName;
   padName << prefix << streamId;
   return padName.str();
}

RefPtr<Pad>
GstRtpSession::createMediaSink(RefPtr<Caps> caps, unsigned int streamId)
{
   RefPtr<Bin> encodeBin = createOutgoingPipeline(caps);

   mMediaBin->add(encodeBin);

   RefPtr<Pad> src = encodeBin->get_static_pad("src");

   RefPtr<Caps> padCaps = Caps::create_from_string("application/x-rtp");
   RefPtr<Gst::PadTemplate> tmpl = PadTemplate::create("send_rtp_sink_%u", PAD_SINK, PAD_REQUEST, padCaps);
   ostringstream padName;
   padName << "sink_" << streamId;
   DebugLog(<< "linking to " << padName.str());
   //RefPtr<Pad> binSink = mRtpTransportBin->create_compatible_pad(queueSrc, caps);
   //RefPtr<Pad> binSink = mRtpTransportBin->get_request_pad(padName.str());
   //RefPtr<Caps> padCaps2 = Caps::create_from_string("application/x-rtp");
   //RefPtr<Pad> binSink = mRtpTransportBin->request_pad(tmpl, padName.str(), padCaps2);
   // is a ghost pad, should have been created already

   RefPtr<Pad> binSink;
   if(mWebRTC)
   {
      // it is a request pad on the webrtcbin
      binSink = mRtpTransportBin->create_compatible_pad(src, caps);
   }
   else
   {
      // it is static_pad on the rtpbin wrapper
      // FIXME - reimplement this as a request pad like webrtcbin
      binSink = mRtpTransportBin->get_static_pad(padName.str());
   }
   if(!binSink)
   {
      CritLog(<<"failed to get request pad " << padName.str());
      resip_assert(0);
   }

   src->link(binSink);

   string sinkName = makePadName("sink_", streamId);
   StackLog(<<"adding new sink: " << sinkName);

   RefPtr<Pad> ghostSink = GhostPad::create(encodeBin->get_static_pad("sink"), sinkName);
   mMediaBin->add_pad(ghostSink);

   return ghostSink;
}

Glib::RefPtr<Gst::Bin>
GstRtpSession::createDecodeBin(const Data& streamKey, const Glib::ustring& srcPadName, bool isWebRTC)
{
   /*RefPtr<Gst::DecodeBin> decodeBin = Gst::DecodeBin::create();*/

   RefPtr<Bin> decodeBin = Gst::Bin::create();
   RefPtr<Element> queue = Gst::Queue::create();
   decodeBin->add(queue);
   RefPtr<Element> dec;
   if(streamKey == "video")
   {
      RefPtr<Element> depay;
      RefPtr<Element> parse;
      if(isWebRTC)
      {
         depay = Gst::ElementFactory::create_element("rtpvp8depay");
         parse = Gst::ElementFactory::create_element("vp8parse");
         dec = Gst::ElementFactory::create_element("vp8dec");
         //dec->property<guint>("threads", 24); // FIXME magic number
      }
      else
      {
         depay = Gst::ElementFactory::create_element("rtph264depay");
         parse = Gst::ElementFactory::create_element("h264parse");
         dec = Gst::ElementFactory::create_element("openh264dec");
      }
      decodeBin->add(depay);
      decodeBin->add(parse);
      decodeBin->add(dec);
      queue->link(depay);
      depay->link(parse);
      parse->link(dec);
   }
   else if(streamKey == "audio")
   {
      RefPtr<Element> depay;
      if(isWebRTC)
      {
         depay = Gst::ElementFactory::create_element("rtpopusdepay");
         dec = Gst::ElementFactory::create_element("opusdec");
      }
      else
      {
         depay = Gst::ElementFactory::create_element("rtppcmadepay");
         dec = Gst::ElementFactory::create_element("alawdec");
      }
      decodeBin->add(depay);
      decodeBin->add(dec);
      queue->link(depay);
      depay->link(dec);
   }
   else
   {
      CritLog(<<"unrecognized streamKey: " << streamKey);
      resip_assert(0);
   }

   RefPtr<Pad> sink = GhostPad::create(queue->get_static_pad("sink"), "sink");
   decodeBin->add_pad(sink);

   sink->signal_linked().connect([this, decodeBin, srcPadName, dec](const RefPtr<Pad>& pad){
      StackLog(<<"linked: " << pad->get_name());
      Iterator<Element> it = decodeBin->iterate_sources();
      it.begin();
      if(it.is_end())
      {
         StackLog(<<"adding src pad");
         // this is where the signal on-pad-added is triggered
         // for any signal handlers created elsewhere
         //RefPtr<Pad> src = GhostPad::create(dec->get_static_pad("src"), "src");
         RefPtr<Pad> src = GhostPad::create(dec->get_static_pad("src"), srcPadName);
         decodeBin->add_pad(src);
      }
      else
      {
         StackLog(<<"src pad already added");
      }
   });

   sink->signal_unlinked().connect([this, decodeBin, dec](const RefPtr<Pad>& pad){
      StackLog(<<"linked: " << pad->get_name());
      Iterator<Element> it = decodeBin->iterate_sources();
      it.begin();
      while(!it.is_end())
      {
         RefPtr<GhostPad> g = RefPtr<GhostPad>::cast_dynamic(*it);
         if(g)
         {
            decodeBin->remove_pad(g);
            g->set_target(RefPtr<Pad>());
            g.clear();
         }
         it.next();
      }
   });

   return decodeBin;
}

#define P_CLIENTS(a,p) (a + ":" + Data(p))  // FIXME - eliminate macros

void
GstRtpSession::createRtpTransportBin()
{
   if(mRtpTransportBin)
   {
      WarningLog(<<"mRtpTransportBin already initialized");
   }

   if(mWebRTC)
   {
      mRtpTransportBin = RefPtr<Bin>::cast_dynamic(ElementFactory::create_element("webrtcbin"));
   }
   else
   {
      mRtpTransportBin = Bin::create();

      // FIXME - find better way to test whether it is already initialized
      /*if(mRtpTransportBin->get_num_children() > 0)
      {
         DebugLog(<<"mRtpTransportBin already initialized, num_children = " << mRtpTransportBin->get_num_children());
         return mRtpTransportBin;
      }*/

      Glib::RefPtr<Gst::Element> rtpBin = Gst::ElementFactory::create_element("rtpbin");
      mRtpTransportBin->add(rtpBin);

      //rtpBin->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added));
      rtpBin->signal_pad_added().connect([this, rtpBin](const RefPtr<Pad>& newPad){
         Glib::ustring padName = newPad->get_name();
         DebugLog(<<"Dynamic pad created. Linking demuxer/decoder " << padName);

         if(newPad->get_direction() != PAD_SRC)
         {
            DebugLog(<<"not a src pad: " << padName);
            return;
         }

         // extract ID
         std::regex r("recv_rtp_src_(\\d+)_(\\d+)_(\\d+)");
         std::smatch match;
         const std::string& s = padName.raw();
         if(std::regex_search(s.begin(), s.end(), match, r))
         {
            DebugLog(<<"found a stream " << padName);
            RefPtr<Pad> ghostSource = GhostPad::create(newPad, padName);
            mRtpTransportBin->add_pad(ghostSource);
            //mSourcePads.push_back(ghostSource);
            return;
         }

         r = "send_rtp_src_(\\d+)";
         if(std::regex_search(s.begin(), s.end(), match, r))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            DebugLog(<<"linking for stream " << streamId);
            RefPtr<Element> udpSink = mSendSinks[streamId];
            rtpBin->link_pads(padName, udpSink, "sink");

            Glib::RefPtr<Gst::Caps> rtcp_caps = Gst::Caps::create_simple(
                     "application/x-rtcp");
            ostringstream _padNameRtcp;
            _padNameRtcp << "send_rtcp_src_" << streamId;
            Glib::ustring padNameRtcp = _padNameRtcp.str();
            DebugLog(<<"RTCP pad: " << padNameRtcp);
            RefPtr<Element> udpRtcpSink = mSendRtcpSinks[streamId];
            //rtpBin->link_pads(padNameRtcp, udpRtcpSink, "sink", rtcp_caps);

            return;
         }

         /*r = "send_rtcp_src_(\\d+)";
         if(std::regex_search(s.begin(), s.end(), match, r))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            DebugLog(<<"linking for stream " << streamId);
            RefPtr<Element> udpRtcpSink = mSendRtcpSinks[streamId];
            rtpBin->link_pads(padName, udpRtcpSink, "sink");
            return;
         }*/

         DebugLog(<<"ignoring pad: " << padName);
         return;

         RefPtr<Pad> sinkPad;

         // extract ID
         //std::regex r("recv_rtp_src_(\\d+)_(\\d+)_(\\d+)");
         //std::smatch match;
         //const std::string& s = padName.raw();
         if (std::regex_search(s.begin(), s.end(), match, r))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            auto& ssrc = match[2];
            StackLog(<<"streamId: " << streamId << " SSRC: " << ssrc);
            RefPtr<Gst::Bin> dec; // = mDecBin.at(streamId);
            sinkPad = dec->get_static_pad("sink");

            // FIXME - setup HOMER if all pads connected
            //
            // Now all audio and video pads, incoming and outgoing,
            // are present.  We can enable the RTCP signals.  If we
            // ask for these signals before the pads are ready then
            // it looks like we don't receive any signals at all.
            //addGstreamerRtcpMonitoringPads(rtpbin, mHepAgent, mPeerSpecs, mCorrelationId);

            PadLinkReturn ret = newPad->link(sinkPad);

            if (ret != PAD_LINK_OK && ret != PAD_LINK_WAS_LINKED)
            {
               DebugLog(<< "Linking of pads " << padName << " and "
                        << sinkPad->get_name() << " failed.");
            }
            DebugLog(<<"linking done");

            /*GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline->gobj()),
            GST_DEBUG_GRAPH_SHOW_ALL,
            "test-pipeline");*/
            /*GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()),
                                         GST_DEBUG_GRAPH_SHOW_ALL,
                                         "test-pipeline");*/
         }
         else
         {
            DebugLog(<<"pad not handled: " << padName);
            return;
         }
      });

      Glib::RefPtr<Gst::Caps> rtcp_caps = Gst::Caps::create_simple("application/x-rtcp");

      // Iterators
      SdpContents::Session::MediumContainer::iterator local = mLocal->session().media().begin(); // FIXME - migrate to const_iterator
      SdpContents::Session::MediumContainer::const_iterator remote = mRemote->session().media().cbegin();
      //std::vector<Glib::RefPtr<Gst::Bin>>::const_iterator enc = mEncBin.cbegin();

      for(; local != mLocal->session().media().end() &&
      remote != mRemote->session().media().cend() /*&&
      enc != mEncBin.cend()*/ ; )
      {
         if(local->name() == "audio" || local->name() == "video")
         {
            string localAddress = local->getConnections().front().getAddress().c_str(); // FIXME - more than one Connection?
            Data peerAddress = remote->getConnections().front().getAddress(); // FIXME - more than one Connection?

            // https://gstreamer.freedesktop.org/documentation/udp/udpsrc.html?gi-language=c
            Glib::RefPtr<Gst::Element> udpSource = Gst::ElementFactory::create_element("udpsrc");
            udpSource->set_property<Glib::ustring>("address", localAddress);
            udpSource->set_property<gint32>("port", local->port());

            Glib::RefPtr<Gst::Element> udpSourceRtcp = Gst::ElementFactory::create_element("udpsrc");
            udpSourceRtcp->set_property<Glib::ustring>("address", localAddress);
            udpSourceRtcp->set_property<gint32>("port", local->firstRtcpPort()); // FIXME - more than one Connection / RTCP?

            // https://gstreamer.freedesktop.org/documentation/udp/multiudpsink.html?gi-language=c#properties
            Glib::RefPtr<Gst::Element> udpSink = Gst::ElementFactory::create_element("multiudpsink");
            Data peerClient = P_CLIENTS(peerAddress, remote->port());
            DebugLog(<<"peerClient = " << peerClient);

            udpSink->set_property<Glib::ustring>("clients", peerClient.c_str());
            // FIXME - specifying the bind-address and bind-port is a very good idea for NAT
            //         traversal (symmetric RTP RFC 4961)
            //         https://www.rfc-editor.org/rfc/rfc4961
            //       - however, when I tried to use these settings, I found that the streams were not
            //         activated at all or sometimes the pad-added event was only received for one
            //         stream (audio or video) and not both
            //       - there is also an option to obtain the GSocket pointer from the udpsrc and give it to
            //         the udpsink so they share the same socket (properties: socket, socket-v6)
            //udpSink->set_property<Glib::ustring>("bind-address", localAddress); // FIXME share socket with udpsrc?
            //udpSink->set_property<gint32>("bind-port", local->port());
            udpSink->set_property("sync", false);
            udpSink->set_property("async", false);
            //udpSink->set_property("qos-dscp", ); // FIXME
            //udpSink->set_property("socket", ); // FIXME
            //udpSink->set_property("socket-v6", ); // FIXME

            Glib::RefPtr<Gst::Element> udpSinkRtcp = Gst::ElementFactory::create_element("multiudpsink");
            Data peerClientRtcp = P_CLIENTS(peerAddress, remote->firstRtcpPort());
            DebugLog(<<"peerClientRtcp = " << peerClientRtcp);

            udpSinkRtcp->set_property<Glib::ustring>("clients", peerClientRtcp.c_str());
            // FIXME - see comments above about bind-address and bind-port with udpSink element
            //udpSinkRtcp->set_property<Glib::ustring>("bind-address", localAddress); // FIXME share socket with udpsrc?
            //udpSinkRtcp->set_property<gint32>("bind-port", local->firstRtcpPort()); // FIXME - more than one Connection / RTCP?
            udpSinkRtcp->set_property("sync", false);
            udpSinkRtcp->set_property("async", false);
            //udpSinkRtcp->set_property("qos-dscp", ); // FIXME
            //udpSinkRtcp->set_property("socket", ); // FIXME
            //udpSinkRtcp->set_property("socket-v6", ); // FIXME

            mRtpTransportBin->add(udpSource);
            mRtpTransportBin->add(udpSourceRtcp);
            mRtpTransportBin->add(udpSink);
            mRtpTransportBin->add(udpSinkRtcp);

            // incoming RTP / RTCP
            Glib::RefPtr<Gst::Caps> incomingCaps = getCaps(*local);
            udpSource->link_pads("src", rtpBin, makePadName("recv_rtp_sink_", mStreamCount), incomingCaps);
            udpSourceRtcp->link_pads("src", rtpBin, makePadName("recv_rtcp_sink_", mStreamCount), rtcp_caps);

            // outgoing RTP - linked by on-pad-added signal handler
            //rtpBin->link_pads(makePadName("send_rtp_src_", mStreamCount), udpSink, "sink");
            // outgoing RTCP - linked manually
            rtpBin->link_pads(makePadName("send_rtcp_src_", mStreamCount), udpSinkRtcp, "sink", rtcp_caps);
            mSendSinks[mStreamCount] = udpSink;
            mSendRtcpSinks[mStreamCount] = udpSinkRtcp;

            // FIXME - create the outgoing ghost pads on request only
            // outgoing media
            /*RefPtr<Gst::Bin> _enc = *enc;
               _enc->link_pads("src", rtpBin, s.str());*/
            //RefPtr<Pad> mediaSink = rtpBin->create_compatible_pad(pad, caps);
            // needs to be consistent with webrtcbin naming convention
            // rather than rtcbin naming conventions
            string rtcBinPadName = makePadName("send_rtp_sink_", mStreamCount);
            string webrtcBinPadName = makePadName("sink_", mStreamCount);
            RefPtr<Pad> mediaSink = rtpBin->get_request_pad(rtcBinPadName);
            RefPtr<Pad> ghostSink = GhostPad::create(mediaSink, webrtcBinPadName);
            mRtpTransportBin->add_pad(ghostSink);

         }

         local++;
         remote++;
         //enc++;
         mStreamCount++;
      }
   }

   mRtpTransportBin->signal_pad_added().connect([this](const RefPtr<Pad>& pad){
      const Glib::ustring padName = pad->get_name();
      if(pad->get_direction() == PAD_SRC)
      {
         StackLog(<<"trying to extract pt from pad name: " << padName);
         std::regex r1("recv_rtp_src_(\\d+)_(\\d+)_(\\d+)");
         std::regex r2("src_(\\d+)");
         std::smatch match;
         const std::string& s = padName.raw();
         // from rtpbin
         if (std::regex_search(s.begin(), s.end(), match, r1))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            unsigned long ssrc = atoi(match[2].str().c_str());
            unsigned int pt = atoi(match[3].str().c_str());
            StackLog(<< "found stream " << streamId
                     << " SSRC: " << ssrc
                     << " payload type: " << pt);

            createDecodeBinForStream(pad, streamId, pt);
         }
         else if (std::regex_search(s.begin(), s.end(), match, r2))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            StackLog(<< "found stream " << streamId);
            createDecodeBinForStream(pad, streamId, -1);
         }
         else
         {
            ErrLog(<<"unrecognized src pad: " << padName);
            return;
         }
      }
      else
      {
         StackLog(<<"not a src pad: " << padName);
      }
   });

   mMediaBin->add(mRtpTransportBin);
}

RefPtr<Bin>
GstRtpSession::getRtpTransportBin()
{
   if(!mRtpTransportBin)
   {
      createRtpTransportBin();
   }

   return mRtpTransportBin;
}

void
GstRtpSession::createDecodeBinForStream(const RefPtr<Pad>& pad, unsigned int streamId, int pt)
{
   // FIXME - some of this logic can move to SdpContents

   Glib::ustring srcPadName = pad->get_name();

   // use streamId to find Medium
   if(!mLocal)
   {
      ErrLog(<<"no local SDP, new pad is not expected and can't be handled");
      return;
   }
   unsigned int mediaCount = mLocal->session().media().size();
   if(streamId >= mediaCount)
   {
      ErrLog(<<"media.size() == " << mediaCount << ", streamId too high: " << streamId);
      return;
   }
   auto mediaIt =  mLocal->session().media().cbegin();
   for(unsigned int i = 0; i < streamId;)
   {
      i++;
      mediaIt++;
   }
   const SdpContents::Session::Medium& m = *mediaIt;

   // use pt to find the format in the Medium
   auto codecIt = m.codecs().cbegin();
   if(pt >= 0)
   {
      while(codecIt != m.codecs().cend() && codecIt->payloadType() != pt)
      {
         if(++codecIt == m.codecs().cend())
         {
            ErrLog(<<"no payload format matches pt: " << pt);
            return;
         }
      }
   }
   else
   {
      RefPtr<Caps> caps = pad->get_current_caps();
      if(!caps->get_structure(0).has_field("encoding-name"))
      {
         CritLog(<<"can't find encoding-name");
         resip_assert(0);
      }
      string capsEncodingName;
      caps->get_structure(0).get_field("encoding-name", capsEncodingName);
      Data _capsEncodingName(capsEncodingName);
      _capsEncodingName.uppercase();
      while(codecIt != m.codecs().cend() && codecIt->getName() != _capsEncodingName)
      {
         if(++codecIt == m.codecs().cend())
         {
            ErrLog(<<"no codec name matches: " << _capsEncodingName);
            return;
         }
      }
   }
   const auto& codec = *codecIt;
   StackLog(<<"matched Codec: " << codec);

   // create the decode Bin
   const CodecConfigMap& codecConfigs = mRTPManager.getCodecConfigMap();
   Data codecName = codec.getName();
   codecName.uppercase(); // FIXME - use a type for case-insensitive codec names
   CodecConfigMap::const_iterator codecRegIt = codecConfigs.find(codecName);
   if(codecRegIt == codecConfigs.end())
   {
      ErrLog(<<"codec " << codecName << " not found in local registry");
      return;
   }
   std::shared_ptr<CodecConfig> cc = codecRegIt->second;
   // FIXME - check all codec properties are compatible

   // create decoder
   RefPtr<Bin> decodeBin = Bin::create();
   RefPtr<Queue> decQueue = Queue::create();
   decodeBin->add(decQueue);
   RefPtr<Element> decPay = ElementFactory::create_element(cc->mDepay.c_str());
   decodeBin->add(decPay);
   decQueue->link(decPay);

   RefPtr<Element> dec = ElementFactory::create_element(cc->mDecoder.c_str());
   decodeBin->add(dec);

   if(!cc->mParse.empty())
   {
      RefPtr<Element> decParse = Gst::ElementFactory::create_element(cc->mParse.c_str());
      decodeBin->add(decParse);
      decPay->link(decParse);
      decParse->link(dec);
   }
   else
   {
      decPay->link(dec);
   }

   bool isVideo = false;   // FIXME - add property to CodecConfig
   if(codecName == "VP8" || codecName == "H264")
   {
      isVideo = true;
   }

   // Setup the keyframe request probe
   if(isVideo)
   {
      addGstreamerKeyframeProbe(pad, [this](){mOnKeyframeRequired();});
   }

   // link the src pad to the decode Bin
   RefPtr<Pad> ghostSink = GhostPad::create(decQueue->get_static_pad("sink"), "sink");
   decodeBin->add_pad(ghostSink);

   RefPtr<Pad> ghostSrc = GhostPad::create(dec->get_static_pad("src"), srcPadName);
   decodeBin->add_pad(ghostSrc);

   // FIXME store it in a map?
   mMediaBin->add(decodeBin);

   // link the src pad from the RtpBin to the DecodeBin
   pad->link(ghostSink);

   // notify the application about the new decode Bin
   // - application receives signal on new src pad here:
   RefPtr<Pad> outerSrc = GhostPad::create(ghostSrc, srcPadName);
   mMediaBin->add_pad(outerSrc);

   if(++mDecodes == getStreamCount())
   {
      DebugLog(<<"all incoming streams connected to decoders");
      if(mHepAgent)
      {
         DebugLog(<<"all pads ready, trying to setup HOMER HEP RTCP");
         //resip::addGstreamerRtcpMonitoringPads(RefPtr<Bin>::cast_dynamic(mRtpTransportElement));
         // Now all audio and video pads, incoming and outgoing,
         // are present.  We can enable the RTCP signals.  If we
         // ask for these signals before the pads are ready then
         // it looks like we don't receive any signals at all.
         RtcpPeerSpecVector peerSpecs = resip::createRtcpPeerSpecs(*getLocalSdp(), *getRemoteSdp());
         addGstreamerRtcpMonitoringPads(RefPtr<Bin>::cast_dynamic(mRtpTransportBin),
            mHepAgent, peerSpecs, mCorrelationId);
      }
   }

   DebugLog(<<"successfully created incoming decode bin for " << srcPadName);
}

void
GstRtpSession::onPlaying()
{
   DebugLog(<<"onPlaying");
   for(auto& pad : mSourcePads)
   {
      DebugLog(<<"onPlaying: adding pad " << pad->get_name());
      //mRtpTransportBin->add_pad(pad);
   }
}

unsigned int
GstRtpSession::getStreamCount()
{
   if(mWebRTC)
   {
      unsigned int result = 0;
      Iterator<Pad> it = mRtpTransportBin->iterate_sink_pads();
      it.begin();
      while(it.next() == ITERATOR_OK)
      {
         result++;
      }
      return result;
   }
   else
   {
      return mStreamCount;
   }
}

void
GstRtpSession::initHomer(const resip::Data& correlationId, std::shared_ptr<resip::HepAgent> hepAgent)
{
   mCorrelationId = correlationId;
   mHepAgent = hepAgent;
}

#endif

/* ====================================================================
 *
 * Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 * Copyright (c) 2022, Daniel Pocock https://danielpocock.com
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */
