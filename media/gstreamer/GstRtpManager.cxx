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

GstRtpManager::GstRtpManager(const resip::Data& localAddress, int portRangeMin, int portRangeMax)
 : mLocalAddress(localAddress),
   mPortManager(portRangeMin, portRangeMax)
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
