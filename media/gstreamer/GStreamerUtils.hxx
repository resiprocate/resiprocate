#ifndef RESIP_GSTREAMER_UTILS
#define RESIP_GSTREAMER_UTILS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_GSTREAMER
#include <map>
#include <utility>
#include <type_traits>
#include <glibmm/main.h>
#include <glibmm/exceptionhandler.h>
#include <glibmm/ustring.h>
#include <gstreamermm.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include "rutil/Subsystem.hxx"
#include "rutil/Log.hxx"
#include "rutil/hep/HepAgent.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/Tuple.hxx"

#include "GstSubsystem.hxx"

extern "C"
{
   resip::Log::Level
   gst_debug_level_to_severity_level (GstDebugLevel level);

   void
   gst2resip_log_function(GstDebugCategory *category, GstDebugLevel level,
                      const gchar *file,
                      const gchar *function, gint line, GObject *object,
                      GstDebugMessage *message, gpointer user_data) G_GNUC_NO_INSTRUMENT;
};

namespace resip
{

static const std::map<GstWebRTCICEConnectionState, const resip::Data> iCEConnectionStates =
         {
                  {GST_WEBRTC_ICE_CONNECTION_STATE_NEW, "new"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_CHECKING, "checking"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED, "connected"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED, "completed"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_FAILED, "failed"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED, "disconnected"},
                  {GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED, "closed"}
         };

static const std::map<GstWebRTCICEGatheringState, const resip::Data> iCEGatheringStates =
         {
                  {GST_WEBRTC_ICE_GATHERING_STATE_NEW, "new"},
                  {GST_WEBRTC_ICE_GATHERING_STATE_GATHERING, "gathering"},
                  {GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE, "complete"}
         };

static const std::map<GstWebRTCSignalingState, const resip::Data> signalingStates =
         {
                  {GST_WEBRTC_SIGNALING_STATE_STABLE, "stable"},
                  {GST_WEBRTC_SIGNALING_STATE_CLOSED, "closed"},
                  {GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_OFFER, "local-offer"},
                  {GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_OFFER, "remote-offer"},
                  {GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_PRANSWER, "local-pranswer"},
                  {GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_PRANSWER, "remote-pranswer"}
         };

template<typename T>
const Data& lookupGstreamerStateName(std::map<T, const Data> m, T v)
{
   auto it = m.find(v);
   if(it == m.end())
   {
      resip_assert(0);
      static Data unknown("UNKNOWN STATE");
      return unknown;
   }
   return it->second;
};

Glib::RefPtr<Gst::Element> buildTestSource(const Data& streamName);

typedef resip::Data MediaTypeName;

MediaTypeName getMediaTypeName(const Glib::RefPtr<Gst::Caps>& caps);

int getStreamIdFromPadName(const Glib::ustring padName);

resip::Data deduceKeyForPadName(const Glib::ustring padName);

/*
 * Must set the environment variable GST_DEBUG_DUMP_DOT_DIR
 * or no Gstreamer graph files will be created.
 *
 * In gdb:
 *       set env GST_DEBUG_DUMP_DOT_DIR=/tmp
 */
void storeGstreamerGraph(Glib::RefPtr<Gst::Bin>& bin, const resip::Data& fileNamePrefix);

void gstWebRTCSetDescription(Glib::RefPtr<Gst::Element> element, const gchar* signalName, GstWebRTCSessionDescription* gstwebrtcdesc);

resip::SdpContents* createSdpContentsFromGstreamer(GstWebRTCSessionDescription *gSdp);

std::shared_ptr<GstWebRTCSessionDescription> createGstWebRTCSessionDescriptionFromSdpContents(GstWebRTCSDPType sdpType, const resip::SdpContents& sdp);

void addGstreamerKeyframeProbe(const Glib::RefPtr<Gst::Pad>& pad, std::function<void()> onKeyframeRequired);

struct RtcpPeerSpec
{
      resip::Tuple local;
      resip::Tuple remote;
};
typedef std::vector<RtcpPeerSpec> RtcpPeerSpecVector;

// call this after all the pads have been created on the rtpbin
bool addGstreamerRtcpMonitoringPads(Glib::RefPtr<Gst::Bin> bin, std::shared_ptr<HepAgent> hepAgent, const RtcpPeerSpecVector& peerSpecs, const Data& correlationId);

// For scenario's where we obtain access to individual elements
// in the rtpbin and insert Tee (GstTee) elements to intercept RTCP flows,
// the pad-added handler must call this to complete the pipeline setup
// and insert an AppSink to handle the RTCP from our new ghost pads.
// This does not need to be called at all if using signals from RTPSession
// to obtain the RTCP.
void linkGstreamerRtcpHomer(Glib::RefPtr<Gst::Bin> bin, Glib::RefPtr<Gst::Pad> pad, std::shared_ptr<resip::HepAgent> hepAgent, const RtcpPeerSpecVector& peerSpecs, const resip::Data& correlationId);

RtcpPeerSpecVector createRtcpPeerSpecs(const SdpContents& local, const SdpContents& remote);

}


namespace Glib
{

template<typename T>
static inline T transform(T p) { return p; }

static inline Glib::RefPtr<Gst::Caps> transform (GstCaps* c) { return Glib::wrap(c, true); }
static inline Glib::RefPtr<Gst::Pad> transform (GstPad* c) { return Glib::wrap(c, true); }
static inline Glib::RefPtr<Gst::Element> transform (GstElement* c) { return Glib::wrap(c, true); }

template<class T>
struct deduce_result_type
{
   template<class U, typename=typename U::element_type::BaseObjectType>
   static typename U::element_type::BaseObjectType* test (int);

   template<class U>
   static U test (...);

   using type = decltype (test<T> (0));
};


template<typename SlotType, typename ...T>
static void signal_callback(GstElement* self, T... x, void* data)
{
   using namespace Gst;
   Gst::Element* obj = dynamic_cast<Gst::Element*>(Glib::ObjectBase::_get_current_wrapper((GObject*) self));
   resip_assert(obj);
   if(obj)
   {
      try
      {
         if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
         {
            (*static_cast<SlotType*>(slot))(transform(x)...);
         }
      }
      catch(...)
      {
         Glib::exception_handlers_invoke();
      }
   }
}

// FIXME - thread safety of map insertion
typedef std::pair<GType,Glib::ustring> SIDictKey;
static std::map<SIDictKey,std::shared_ptr<SignalProxyInfo>> signalProxyInfoDict;

template<typename ReturnType, typename ...T>
static Glib::SignalProxy< ReturnType, T... > signal_any(const Glib::RefPtr<Gst::Element>& element, const Glib::ustring name)
{
   std::shared_ptr<SignalProxyInfo> info;
   SIDictKey key(element->get_type(), name);
   if(signalProxyInfoDict.find(key) != signalProxyInfoDict.cend())
   {
      info = signalProxyInfoDict.at(key);
   }
   else
   {
      info.reset(new SignalProxyInfo
               {
                  name.c_str(),
                  (GCallback) &signal_callback<sigc::slot< ReturnType, T...>, typename deduce_result_type<T>::type...>,
                  (GCallback) &signal_callback<sigc::slot< ReturnType, T...>, typename deduce_result_type<T>::type...>
               });
      signalProxyInfoDict[key] = info;
   }

   return Glib::SignalProxy< ReturnType,T...>(element.operator->(), info.get());
}

}



#endif

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
