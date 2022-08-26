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

GstRtpSession::GstRtpSession(GstRtpManager& rTPManager)
 : mRTPManager(rTPManager)
{
   mOuterBin = Gst::Bin::create();
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
      Glib::RefPtr<Gst::Bin> encBin;
      Glib::RefPtr<Gst::Bin> decBin;
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

            // create encoder
            encBin = Gst::Bin::create();
            Glib::RefPtr<Gst::Element> enc = Gst::ElementFactory::create_element(cc.mEncoder.c_str());
            Glib::RefPtr<Gst::Element> encPay = Gst::ElementFactory::create_element(cc.mPay.c_str());
            Glib::RefPtr<Gst::Queue> encQueue = Gst::Queue::create();
            encBin->add(encQueue);
            encBin->add(enc);
            encBin->add(encPay);
            enc->link(encPay);
            encPay->link(encQueue);

            RefPtr<Pad> ghostSink = GhostPad::create(enc->get_static_pad("sink"), "sink");
            encBin->add_pad(ghostSink);
            RefPtr<Pad> ghostSource = GhostPad::create(encQueue->get_static_pad("src"), "src");
            encBin->add_pad(ghostSource);

            // create decoder
            decBin = Gst::Bin::create();
            Glib::RefPtr<Gst::Queue> decQueue = Gst::Queue::create();
            Glib::RefPtr<Gst::Element> decPay = Gst::ElementFactory::create_element(cc.mDepay.c_str());
            Glib::RefPtr<Gst::Element> decParse;
            Glib::RefPtr<Gst::Element> dec = Gst::ElementFactory::create_element(cc.mDecoder.c_str());

            if(!cc.mParse.empty())
            {
               decParse = Gst::ElementFactory::create_element(cc.mParse.c_str());
               decBin->add(decParse);
            }

            decBin->add(decQueue);
            decBin->add(decPay);
            decBin->add(dec);
            decQueue->link(decPay);
            if(decParse)
            {
               decPay->link(decParse);
               decParse->link(dec);
            }
            else
            {
               decPay->link(dec);
            }
            ghostSink = GhostPad::create(decQueue->get_static_pad("sink"), "sink");
            decBin->add_pad(ghostSink);
            ghostSource = GhostPad::create(dec->get_static_pad("src"), "src");
            decBin->add_pad(ghostSource);

            //mOuterBin->add(encBin);  // FIXME
            //mOuterBin->add(decBin);  // FIXME
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
      mEncBin.push_back(encBin);
      mDecBin.push_back(decBin);
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

std::string
makePadName(const std::string& prefix, unsigned int streamId)
{
   ostringstream padName;
   padName << prefix << streamId;
   return padName.str();
}

#define P_CLIENTS(a,p) (a + ":" + Data(p))  // FIXME - eliminate macros

Glib::RefPtr<Gst::Bin>
GstRtpSession::createRtpBinOuter()
{
   // FIXME - find better way to test whether it is already initialized
   /*if(mOuterBin->get_num_children() > 0)
   {
      DebugLog(<<"mOuterBin already initialized, num_children = " << mOuterBin->get_num_children());
      return mOuterBin;
   }*/

   Glib::RefPtr<Gst::Element> rtpBin = Gst::ElementFactory::create_element("rtpbin");
   mOuterBin->add(rtpBin);

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
         mOuterBin->add_pad(ghostSource);
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
         RefPtr<Gst::Bin> dec = mDecBin.at(streamId);
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
   unsigned int streamId = 0;

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

         mOuterBin->add(udpSource);
         mOuterBin->add(udpSourceRtcp);
         mOuterBin->add(udpSink);
         mOuterBin->add(udpSinkRtcp);

         // incoming RTP / RTCP
         Glib::RefPtr<Gst::Caps> incomingCaps = getCaps(*local);
         udpSource->link_pads("src", rtpBin, makePadName("recv_rtp_sink_", streamId), incomingCaps);
         udpSourceRtcp->link_pads("src", rtpBin, makePadName("recv_rtcp_sink_", streamId), rtcp_caps);

         // outgoing RTP - linked by on-pad-added signal handler
         //rtpBin->link_pads(makePadName("send_rtp_src_", streamId), udpSink, "sink");
         // outgoing RTCP - linked manually
         rtpBin->link_pads(makePadName("send_rtcp_src_", streamId), udpSinkRtcp, "sink", rtcp_caps);
         mSendSinks[streamId] = udpSink;
         mSendRtcpSinks[streamId] = udpSinkRtcp;

         // outgoing media
         /*RefPtr<Gst::Bin> _enc = *enc;
         _enc->link_pads("src", rtpBin, s.str());*/
         //RefPtr<Pad> mediaSink = rtpBin->create_compatible_pad(pad, caps);
         // needs to be consistent with webrtcbin naming convention
         // rather than rtcbin naming conventions
         string rtcBinPadName = makePadName("send_rtp_sink_", streamId);
         string webrtcBinPadName = makePadName("sink_", streamId);
         RefPtr<Pad> mediaSink = rtpBin->get_request_pad(rtcBinPadName);
         RefPtr<Pad> ghostSink = GhostPad::create(mediaSink, webrtcBinPadName);
         mOuterBin->add_pad(ghostSink);

      }

      local++;
      remote++;
      //enc++;
      streamId++;
   }

   return mOuterBin;
}

void
GstRtpSession::onPlaying()
{
   DebugLog(<<"onPlaying");
   for(auto& pad : mSourcePads)
   {
      DebugLog(<<"onPlaying: adding pad " << pad->get_name());
      //mOuterBin->add_pad(pad);
   }
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
