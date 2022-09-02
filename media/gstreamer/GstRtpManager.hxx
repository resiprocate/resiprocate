#ifndef RESIP_GSTRTPMANAGER
#define RESIP_GSTRTPMANAGER

#ifdef HAVE_CONFIG_H
#include "../../config.h"
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

#include "../../rutil/Subsystem.hxx"
#include "../../rutil/Log.hxx"
#include "../../rutil/hep/HepAgent.hxx"
#include "../../resip/stack/SdpContents.hxx"
#include "../../resip/stack/Tuple.hxx"
#include "../../rutil/hep/HepAgent.hxx"

#include "../RTPPortManager.hxx"

namespace resipgst
{

class CodecConfig
{
   public:
      CodecConfig(const resip::Data& name,
                  const resip::Data& decoder,
                  const resip::Data& encoder,
                  const resip::Data& h264Profile,
                  const resip::Data& depay,
                  const resip::Data& pay,
                  const resip::Data& parse,
                  const unsigned int rate,
                  const resip::Data& fmtp) :
         mName(name),
         mDecoder(decoder), mEncoder(encoder), mH264Profile(h264Profile),
         mDepay(depay), mPay(pay), mParse(parse), mRate(rate), mFmtp(fmtp) {}
      resip::Data mName;
      resip::Data mDecoder;
      resip::Data mEncoder;
      resip::Data mH264Profile;
      resip::Data mDepay;
      resip::Data mPay;
      resip::Data mParse;
      unsigned int mRate;
      resip::Data mFmtp;
};
typedef std::map<resip::Data,std::shared_ptr<CodecConfig>> CodecConfigMap;

class GstRtpManager
{
   public:
      GstRtpManager(const resip::Data& localAddress);
      virtual ~GstRtpManager();

      const resip::Data& getLocalAddress() const { return mLocalAddress; };
      resip::RTPPortManager& getPortManager() { return mPortManager; };

      const CodecConfigMap& getCodecConfigMap() { return mCodecConfigMap; };

   protected:
      virtual void initCodecConfigMap();

   private:
      resip::Data mLocalAddress;
      resip::RTPPortManager mPortManager;
      CodecConfigMap mCodecConfigMap;
};

class GstRtpSession
{
   public:
      GstRtpSession(GstRtpManager& rTPManager, bool webRTC);
      virtual ~GstRtpSession();
      virtual std::shared_ptr<resip::SdpContents> buildOffer(bool audio, bool video);
      virtual std::shared_ptr<resip::SdpContents> buildAnswer(std::shared_ptr<resip::SdpContents> remoteOffer);
      virtual void processAnswer(std::shared_ptr<resip::SdpContents> remoteAnswer);

      virtual Glib::RefPtr<Gst::Caps> getCaps(resip::SdpContents::Session::Medium& m);
      // FIXME - assumes only one medium for each name
      virtual Glib::RefPtr<Gst::Caps> getCaps(const resip::Data& mediumName);

      Glib::RefPtr<Gst::Bin> createOutgoingPipeline(const Glib::RefPtr<Gst::Caps> caps);
      Glib::RefPtr<Gst::Bin> createDecodeBin(const resip::Data& streamKey, const Glib::ustring& srcPadName, bool isWebRTC);
      Glib::RefPtr<Gst::Pad> createMediaSink(Glib::RefPtr<Gst::Caps> caps, unsigned int streamId);

      virtual Glib::RefPtr<Gst::Bin> getMediaBin() { return mMediaBin; };
      //virtual void setRtpTransportBin(Glib::RefPtr<Gst::Bin> bin);
      virtual Glib::RefPtr<Gst::Bin> getRtpTransportBin();

      // FIXME - use a signal handler to invoke this, make it private
      virtual void onPlaying();

      std::shared_ptr<resip::SdpContents> getLocalSdp() const { return mLocal; };
      void setLocalSdp(std::shared_ptr<resip::SdpContents> local) { mLocal = local; };
      std::shared_ptr<resip::SdpContents> getRemoteSdp() const { return mRemote; };
      void setRemoteSdp(std::shared_ptr<resip::SdpContents> remote) { mRemote = remote; };

      virtual bool isWebRTC() const { return mWebRTC; };

      virtual void setKeyframeRequestHandler(std::function<void()> onKeyframeRequired) { mOnKeyframeRequired = onKeyframeRequired; };

      virtual unsigned int getStreamCount();

      virtual void initHomer(const resip::Data& correlationId, std::shared_ptr<resip::HepAgent> hepAgent);

   private:
      virtual void createRtpTransportBin();
      virtual void createDecodeBinForStream(const Glib::RefPtr<Gst::Pad>& pad, unsigned int streamId, int pt);

      const resip::Data& getLocalAddress() const;
      unsigned int allocatePort();

      GstRtpManager& mRTPManager;
      bool mWebRTC;

      std::shared_ptr<resip::SdpContents> mLocal;
      std::shared_ptr<resip::SdpContents> mRemote;

      // contains the encode and decodes and the transport wrapper
      Glib::RefPtr<Gst::Bin> mMediaBin;
      // wraps the transport (webrtcbin or rtpbin)
      Glib::RefPtr<Gst::Bin> mRtpTransportBin;

      std::vector<Glib::RefPtr<Gst::Pad> > mSourcePads;
      std::map<unsigned int, Glib::RefPtr<Gst::Element> > mSendSinks;
      std::map<unsigned int, Glib::RefPtr<Gst::Element> > mSendRtcpSinks;

      unsigned int mStreamCount = 0;
      unsigned int mDecodes = 0;

      std::function<void()> mOnKeyframeRequired;

      // for HOMER / HEP / EEP
      resip::Data mCorrelationId;
      std::shared_ptr<resip::HepAgent> mHepAgent;
};

}

#endif

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
