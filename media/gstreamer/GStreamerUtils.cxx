#include "GStreamerUtils.hxx"

#ifdef USE_GSTREAMER

#include <glib.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <glibmm.h>
#include <gstreamermm.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/gstwebrtcbin.h>

#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

#include "resip/stack/Tuple.hxx"

using namespace resip;
using namespace Glib;
using namespace Gst;

#define RESIPROCATE_SUBSYSTEM resip::GstSubsystem::GSTREAMER

GstSubsystem GstSubsystem::GSTREAMER("GSTREAMER");

extern "C"
{

Log::Level
gst_debug_level_to_severity_level (GstDebugLevel level)
{
   switch (level) 
   {
      case GST_LEVEL_ERROR:   return Log::Err;
      case GST_LEVEL_WARNING: return Log::Warning;
      case GST_LEVEL_FIXME:   return Log::Info;
      case GST_LEVEL_INFO:    return Log::Info;
      case GST_LEVEL_DEBUG:   return Log::Debug;
      case GST_LEVEL_LOG:     return Log::Stack;
      case GST_LEVEL_TRACE:   return Log::Stack;
      default:                return Log::None;
   }
}

void
gst2resip_log_function(GstDebugCategory *category, GstDebugLevel level,
                   const gchar *file,
                   const gchar *function, gint line, GObject *object,
                   GstDebugMessage *message, gpointer user_data)
{
   if (level > gst_debug_category_get_threshold (category) ) 
   {
      return;
   }

   Log::Level level_ = gst_debug_level_to_severity_level (level);

   if (level_ == Log::None) 
   {
      return;
   }

   Subsystem& system_ = RESIPROCATE_SUBSYSTEM;
   do
   {
      if (genericLogCheckLevel(level_, system_))
      {
         resip::Log::Guard _resip_log_guard(level_, system_, file, line, function);
         _resip_log_guard.asStream() << "[" << category->name << "]: ";
         // FIXME - include the GObject *object with debug_object (object)
         _resip_log_guard.asStream() << gst_debug_message_get (message);
      }
   }
   while(false);
}

} // extern "C"

void resip::storeGstreamerGraph(RefPtr<Gst::Bin>& bin, const resip::Data& fileNamePrefix)
{
   GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(bin->gobj(), GST_DEBUG_GRAPH_SHOW_ALL, fileNamePrefix.c_str());
}

void resip::gstWebRTCSetDescription(Glib::RefPtr<Element> element, const gchar* signalName, GstWebRTCSessionDescription* gstwebrtcdesc)
{
   GstPromise* promise = gst_promise_new();
   g_signal_emit_by_name (element->gobj(), signalName, gstwebrtcdesc, promise);
   gst_promise_interrupt(promise);
   gst_promise_unref(promise);
}

SdpContents*
resip::createSdpContentsFromGstreamer(GstWebRTCSessionDescription *gSdp)
{
   gchar *text = gst_sdp_message_as_text (gSdp->sdp);
   StackLog(<<"text from gst_sdp_message_as_text: " << text);
   HeaderFieldValue hfv(text, strlen(text));
   Mime type("application", "sdp");
   SdpContents *result = dynamic_cast<SdpContents*>((new SdpContents(hfv, type))->clone());
   g_free(text);
   return result;
}

std::shared_ptr<GstWebRTCSessionDescription>
resip::createGstWebRTCSessionDescriptionFromSdpContents(GstWebRTCSDPType sdpType, const SdpContents& sdp)
{
   std::ostringstream sdpBuf;
   sdpBuf << sdp;

   GstSDPMessage* _gstdesc = nullptr;
   GstSDPResult r = gst_sdp_message_new_from_text(sdpBuf.str().c_str(), &_gstdesc);
   if(r != GST_SDP_OK || !_gstdesc)
   {
      ErrLog(<<"failed to create GstSDPMessage");
      return nullptr;
   }
   GstWebRTCSessionDescription *_gstwebrtc = nullptr;
   _gstwebrtc = gst_webrtc_session_description_new (sdpType, _gstdesc);
   std::shared_ptr<GstWebRTCSessionDescription> gstwebrtc(_gstwebrtc, [](GstWebRTCSessionDescription* p){gst_webrtc_session_description_free(p);});
   return gstwebrtc;
}

void resip::addGstreamerKeyframeProbe(const Glib::RefPtr<Gst::Pad>& pad, std::function<void()> onKeyframeRequired)
{
   pad->add_probe(PAD_PROBE_TYPE_EVENT_UPSTREAM,
            [onKeyframeRequired](const Glib::RefPtr<Gst::Pad>& pad, const Gst::PadProbeInfo& info)
   {
      DebugLog(<<"received event on video stream");
      RefPtr<Gst::Event> event = info.get_event();
      if(event &&
               event->get_event_type() == EVENT_CUSTOM_UPSTREAM &&
               event->has_name("GstForceKeyUnit"))
      {
         DebugLog(<<"gstreamer decoder wants a keyframe");
         onKeyframeRequired();
         return PAD_PROBE_HANDLED;
      }
      return PAD_PROBE_PASS;
   });
}

// logic for inserting the Tee elements in the RTCP flow and
// creating ghost pads to access the RTCP flow from the application
bool addGstreamerRtcpMonitoringPadsInternal(RefPtr<Bin>& outerBin, RefPtr<Bin>& rtpbin, Gst::Iterator<Gst::Pad> it, ustring prefix)
{
   RefPtr<Caps> rtcpCaps = Gst::Caps::create_from_string("application/x-rtcp");
   RefPtr<Gst::PadTemplate> padTemplate = PadTemplate::create("src_%", PAD_SRC, PAD_REQUEST, rtcpCaps);
   while(it.next())
   {
      RefPtr<Gst::Pad> src = *it;
      RefPtr<Pad> peer = src->get_peer();
      if((src->get_name().find("rtcp") != ustring::npos) && peer)
      {
         RefPtr<Element> tee = ElementFactory::create_element("tee");
         rtpbin->add(tee);
         ustring proxyName = prefix + src->get_name();
         RefPtr<Pad> ghost = GhostPad::create(tee->request_pad(padTemplate), proxyName);
         rtpbin->add_pad(ghost);
         src->unlink(peer);
         RefPtr<Pad> teeSrc1 = tee->request_pad(padTemplate);
         teeSrc1->link(peer);
         src->link(tee->get_static_pad("sink"));

         if(outerBin != rtpbin)
         {
            // rtpbin is nested inside webrtcbin
            RefPtr<GhostPad> outerGhost = GhostPad::create(ghost, proxyName);
            outerBin->add_pad(outerGhost);
         }

         tee->sync_state_with_parent();
      }
   }
   return true;
}

// the internal RTPSession class
// https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSession.html?gi-language=c#RTPSession
struct RTPSession;

// stub function to proxy signal from C-style handler to C++
typedef std::function<void(GstBuffer*)> RxRTCPCallback;
void rxRtcpStub(GstElement* session, GstBuffer* buffer, void *u)
{
   RxRTCPCallback &func = *static_cast<RxRTCPCallback*>(u);
   func(buffer);
}

// stub function to proxy signal from C-style handler to C++
typedef std::function<gboolean(GstBuffer*, gboolean)> TxRTCPCallback;
gboolean txRtcpStub(GstElement* session, GstBuffer* buffer, gboolean early, void *u)
{
   TxRTCPCallback &func = *static_cast<TxRTCPCallback*>(u);
   return func(buffer, early);
}

guint processReceiverReports(DataStream& stream, GstRTCPPacket *_packet)
{
   guint count, i;

   stream << "\"report_blocks\":[";

   count = gst_rtcp_packet_get_rb_count (_packet);
   for (i = 0; i < count; i++)
   {
      guint32 ssrc, exthighestseq, jitter, lsr, dlsr;
      guint8 fractionlost;
      gint32 packetslost;

      gst_rtcp_packet_get_rb (_packet, i, &ssrc, &fractionlost,
         &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);

      stream << (i>0 ? ",":"") << "{"
             << "\"source_ssrc\":" << ssrc << ","
             << "\"highest_seq_no\":" << exthighestseq << ","
             << "\"fraction_lost\":" << (uint32_t)fractionlost << ","
             << "\"ia_jitter\":" << jitter << ","
             << "\"packets_lost\":" << packetslost << ","
             << "\"lsr\":" << lsr << ","
             << "\"dlsr\":" << dlsr
             << "}";
   }

   stream << "],\"report_count\":" << count;

   return count;
}

void onSenderReport(DataStream& stream, GstRTCPPacket *_packet)
{
   guint32 ssrc;
   guint64 ntptime;
   guint32 rtptime;
   guint32 packet_count;
   guint32 octet_count;
   gst_rtcp_packet_sr_get_sender_info(_packet, &ssrc, &ntptime, &rtptime, &packet_count, &octet_count);

   stream << "\"sender_information\":{"
          << "\"ntp_timestamp_sec\":" << (ntptime >> 32) << ","
          << "\"ntp_timestamp_usec\":" << (ntptime & 0xffff) << ","
          << "\"octets\":" << octet_count << ","
          << "\"rtp_timestamp\":" << rtptime << ","
          << "\"packets\":" << packet_count
          << "}";

   stream << ",\"ssrc\":" << ssrc
          << ",\"type\":" << 200;

   stream << ",";
   processReceiverReports(stream, _packet);
}

void onReceiverReport(DataStream& stream, GstRTCPPacket *_packet)
{
   guint32 senderssrc;

   senderssrc = gst_rtcp_packet_rr_get_ssrc (_packet);

   stream << "\"ssrc\":" << senderssrc
          << ",\"type\":" << 201;

   stream << ",";
   processReceiverReports(stream, _packet);
}

void onBuffer(GstBuffer *__buffer, const GenericIPAddress& source, const GenericIPAddress& destination, std::shared_ptr<HepAgent> hepAgent, const Data& correlationId)
{
   if(!__buffer)
   {
      ErrLog(<<"__buffer is NULL");
      return;
   }

   gpointer iteratorState = nullptr;
   GstMeta* _m;
   while((_m = gst_buffer_iterate_meta(__buffer, &iteratorState)))
   {
      //RefPtr<Gst::Meta> m = Meta::create(_m);
      // we receive GstMeta on some packets using regular rtpbin/udpsrc/udpsink
      // we don't receive any GstMeta from webrtcbin
      StackLog(<<"found GstMeta on RTCP");
   }

   // Use reSIProcate's HepAgent code to extract values from RTCP

   // This doesn't work right now, possibly because of issues in struct
   // alignment or byte order.  Therefore, we use the Gstreamer API to
   // examine the RTCP buffer.
   /*{
      RefPtr<Buffer> buffer = Glib::wrap(__buffer, true);
      gsize packetSize = buffer->get_size();
      StackLog(<<"got an RTCP packet, packetSize = " << packetSize);
      char* _buffer = new char[packetSize];
      gsize extracted = buffer->extract(0, _buffer, packetSize);
      StackLog(<<"extracted bytes: " << extracted);
      Data packet(Data::Take, _buffer, packetSize);
      // send to HEP
      hepAgent->sendRTCP(TransportType::UDP, source, destination, packet, correlationId);
      return;
   }*/

   // use Gstreamer RTCP code to extract values from RTCP

   if (!gst_rtcp_buffer_validate_reduced (__buffer))
   {
      ErrLog(<<"_buffer validation failed");
      return;
   }
   GstRTCPBuffer _rtcp = { NULL, };
   gboolean result = gst_rtcp_buffer_map(__buffer, GST_MAP_READ, &_rtcp);
   if(!result)
   {
      ErrLog(<<"gst_rtcp_buffer_map failed");
      //return;
   }

   GstRTCPPacket _packet;
   gboolean more = gst_rtcp_buffer_get_first_packet(&_rtcp, &_packet);
   while(more)
   {
      Data json;
      DataStream stream(json);

      stream << "{";

      GstRTCPType type;
      type = gst_rtcp_packet_get_type (&_packet);
      StackLog(<<"processing a GstRTCPPacket, type: " << type);
      switch (type)
      {
      case GST_RTCP_TYPE_SR:
         onSenderReport(stream, &_packet);
         break;
      case GST_RTCP_TYPE_RR:
         onReceiverReport(stream, &_packet);
         break;
      default:
         WarningLog(<<"unhandled RTCP packet type: " << type);
      }

      stream << "}";
      stream.flush();

      if(json.size() > 2)
      {
         StackLog(<< "created JSON for HOMER: " << json);
         hepAgent->sendToHOMER<Data>(TransportType::UDP, source, destination, HepAgent::RTCP_JSON, json, correlationId);
      }
      else
      {
         ErrLog(<< "no JSON created");
      }

      more = gst_rtcp_packet_move_to_next (&_packet);
   }

   gst_rtcp_buffer_unmap(&_rtcp);
}

bool resip::addGstreamerRtcpMonitoringPads(Glib::RefPtr<Gst::Bin> bin, std::shared_ptr<HepAgent> hepAgent, const RtcpPeerSpecVector& peerSpecs, const Data& correlationId)
{
   if(!hepAgent)
   {
      WarningLog(<<"HepAgent is NULL");
      return false;
   }
   RefPtr<Bin> rtpbin;
   //if(bin->get_type() == gst_webrtc_bin_get_type())
   if(bin->get_name() == "webrtcbin0")
   {
      rtpbin = RefPtr<Bin>::cast_dynamic(bin->get_element("rtpbin"));
   }
   else if(bin->get_name() == "rtpbin" || bin->get_name() == "rtpbin0")
   {
      rtpbin = bin;
   }
   if(!rtpbin)
   {
      ErrLog(<<"failed to find rtpbin");
      return false;
   }

   // Obtain access to individual elements in the rtpbin and insert
   // Tee (GstTee) elements to intercept RTCP flows.
   /* RefPtr<Element> session = rtpbin->get_element("rtpsession0");
   if(!rtpbin)
   {
      ErrLog(<<"failed to find rtpsession0");
      return false;
   }
   RefPtr<Element> demux = rtpbin->get_element("rtpssrcdemux0");
   if(!rtpbin)
   {
      ErrLog(<<"failed to find rtpssrcdemux0");
      return false;
   } */

   // incoming RTCP
   /* if(!addGstreamerRtcpMonitoringPadsInternal(bin, rtpbin, demux->iterate_src_pads(), "inbound_"))
   {
      ErrLog(<<"inbound linking failed");
      return false;
   }

   // outgoing RTCP
   if(!addGstreamerRtcpMonitoringPadsInternal(bin, rtpbin, session->iterate_src_pads(), "outbound_"))
   {
      ErrLog(<<"outbound linking failed");
      return false;
   }*/

   StackLog(<<"peerSpecs.size(): " << peerSpecs.size());
   resip_assert(peerSpecs.size() > 0);
   guint sessionId = 0;
   {
      const RtcpPeerSpec& ps = peerSpecs[sessionId];
      GenericIPAddress local = ps.local.toGenericIPAddress();
      GenericIPAddress remote = ps.remote.toGenericIPAddress();
      RTPSession *_session;
      g_signal_emit_by_name (rtpbin->gobj(), "get-internal-session", sessionId, &_session);
      if(_session)
      {

         // FIXME shared_ptr
         RxRTCPCallback *rxcb = new RxRTCPCallback([hepAgent, local, remote, correlationId](GstBuffer *buffer)
         {
            DebugLog(<<"on-receiving-rtcp invoked");
            onBuffer(buffer, remote, local, hepAgent, correlationId);
         });

         TxRTCPCallback *txcb = new TxRTCPCallback([hepAgent, local, remote, correlationId](GstBuffer *buffer, gboolean early) -> gboolean
         {
            DebugLog(<<"on-sending-rtcp invoked");
            onBuffer(buffer, local, remote, hepAgent, correlationId);
            return TRUE;
         });

         // FIXME - need to wrap internal RTPSession type
         /*RefPtr<Glib::Object> __session = Glib::wrap(G_OBJECT(_session), false);
         auto s1 = Glib::signal_any<void,GstBuffer*>(__session, "on-receiving-rtcp").connect(rxcb);
         auto s2 = Glib::signal_any<void,GstBuffer*, gboolean>(__session, "on-sending-rtcp").connect(txcb);*/
         g_signal_connect_after (_session, "on-receiving-rtcp",
            G_CALLBACK (&rxRtcpStub), rxcb);
         g_signal_connect_after (_session, "on-sending-rtcp",
            G_CALLBACK (&txRtcpStub), txcb);

         DebugLog(<<"RTCP monitoring setup for session " << sessionId);
      }
      else
      {
         WarningLog(<<"failed to obtain RTPSession for sessionId " << sessionId);
      }
   }
   return true;
}

void resip::linkGstreamerRtcpHomer(Glib::RefPtr<Gst::Bin> bin, Glib::RefPtr<Gst::Pad> pad, std::shared_ptr<HepAgent> hepAgent, const RtcpPeerSpecVector& peerSpecs, const Data& correlationId)
{
   // find index for tuple
   // std::bind the incoming/outgoing
   GenericIPAddress source;
   GenericIPAddress destination;
   StackLog(<<"peerSpecs.size(): " << peerSpecs.size());
   resip_assert(peerSpecs.size() > 0);
   const RtcpPeerSpec& ps = peerSpecs.front();

   // examine pad
   // in or out?
   Glib::ustring padName = pad->get_name();
   bool inbound = true;
   if(padName.substr(0, 9) == "outbound_")
   {
      inbound = false;
      source = ps.local.toGenericIPAddress();
      destination = ps.remote.toGenericIPAddress();
   }
   else if(padName.substr(0, 8) == "inbound_")
   {
      source = ps.remote.toGenericIPAddress();
      destination = ps.local.toGenericIPAddress();
   }
   else
   {
      resip_assert(0); // FIXME
   }
   DebugLog(<<"RTCP adding probe for pad " << padName
      << " source " << source
      << " destination " << destination
      << " direction " << (inbound ? "inbound" : "outbound"));

   RefPtr<AppSink> appSink = AppSink::create();
   bin->add(appSink);
   appSink->property_emit_signals() = true;
   appSink->signal_new_sample().connect([hepAgent, peerSpecs, pad, appSink, source, destination, correlationId]()
            -> Gst::FlowReturn {
      RefPtr<Sample> sample = appSink->pull_sample();
      StackLog(<<"got an RTCP sample, caps = " << sample->get_caps()->to_string());
      RefPtr<Buffer> buffer = sample->get_buffer();
      if(!buffer)
      {
         ErrLog(<<"sample doesn't contain a buffer");
         return FLOW_OK;
      }
      onBuffer(buffer->gobj(), source, destination, hepAgent, correlationId);
      return FLOW_OK;
   });
   pad->link(appSink->get_static_pad("sink"));
   appSink->sync_state_with_parent();
}

Tuple createRtcpTuple(const SdpContents::Session::Medium& m)
{
   auto& c = m.getConnections().front();
   TransportType tt = UDP;  // FIXME - check for valid m.protocol()
   return Tuple(c.getAddress(), m.firstRtcpPort(), tt);
}

RtcpPeerSpecVector resip::createRtcpPeerSpecs(const SdpContents& local, const SdpContents& remote)
{
   RtcpPeerSpecVector result;

   SdpContents::Session::MediumContainer::const_iterator iLocal = local.session().media().cbegin();
   SdpContents::Session::MediumContainer::const_iterator iRemote = remote.session().media().cbegin();

   while((iLocal != local.session().media().cend()) &&
        (iRemote != remote.session().media().cend()))
   {
      result.push_back({
         createRtcpTuple(*iLocal),
         createRtcpTuple(*iRemote)
      });

      iLocal++;
      iRemote++;
   }

   return result;
}


#endif

/* ====================================================================
 *
 * Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 * Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
