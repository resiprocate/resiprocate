#include "GStreamerUtils.hxx"

#ifdef USE_GSTREAMER

#include <gst/gst.h>
#include <glibmm.h>
#include <gstreamermm.h>

#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

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

void resip::addGstreamerKeyframeProbe(Glib::RefPtr<Gst::Pad>& pad, std::function<void()> onKeyframeRequired)
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
