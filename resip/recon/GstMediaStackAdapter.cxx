
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

// resip includes
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Lock.hxx>
#include <rutil/Random.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>

#include "ReconSubsystem.hxx"
#include "UserAgent.hxx"
#include "GstMediaStackAdapter.hxx"
#include "ConversationManagerCmds.hxx"
#include "GstConversation.hxx"
#include "Participant.hxx"
#include "GstBridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "GstRemoteParticipant.hxx"
#include "Conversation.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <media/gstreamer/GStreamerUtils.hxx>

#include "cajun/json/elements.h"

#include <glibmm/main.h>
#include <gstreamermm.h>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

using namespace Gst;
using Glib::RefPtr;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

GstMediaStackAdapter::GstMediaStackAdapter(ConversationManager& conversationManager)
: MediaStackAdapter(conversationManager),
  mGstTOSValue(0) // FIXME - make this value configurable
{
   init();
}

GstMediaStackAdapter::GstMediaStackAdapter(ConversationManager& conversationManager, int defaultSampleRate, int maxSampleRate)
: MediaStackAdapter(conversationManager),
  mGstTOSValue(0) // FIXME - make this value configurable
{
   init(defaultSampleRate, maxSampleRate);
}

void
GstMediaStackAdapter::init(int defaultSampleRate, int maxSampleRate)
{
   DebugLog(<<"Gstreamer media stack initialization");

   const char* _argv[] =  { "reConServer", "--gst-debug", "5", NULL }; // FIXME
   int _argc = 3;
   char** __argv = (char**)_argv;
   // FIXME: are there C++ equivalents of these functions?
   gst_debug_remove_log_function (gst_debug_log_default);
   gst_debug_add_log_function(gst2resip_log_function, nullptr, nullptr);
   Gst::init(_argc, __argv);

   main_loop = Glib::MainLoop::create();

   //run();
}

void
GstMediaStackAdapter::runMainLoop()
{
   run();  // FIXME: run one Gst::main_loop thread per process or per Conversation?
}

void
GstMediaStackAdapter::thread()
{
   DebugLog(<<"running the Gstreamer main loop");
   main_loop->run();
   DebugLog(<<"main_loop::run() completed");
}

void
GstMediaStackAdapter::shutdown()
{
   InfoLog(<<"GstMediaStackAdapter shutdown");
   ThreadIf::shutdown();
   main_loop->quit();
   // FIXME - join / wait for any threads to stop
}

GstMediaStackAdapter::~GstMediaStackAdapter()
{
   // FIXME - move to ::shutdown() ?
   getBridgeMixer().reset();   // Make sure the mixer is destroyed before the media interface
   shutdown();
   join();
}

void
GstMediaStackAdapter::conversationManagerReady(ConversationManager* conversationManager)
{
}

void
GstMediaStackAdapter::setUserAgent(UserAgent* userAgent)
{
   // FIXME for Gst
//   if (mMediaInterface)
//   {
//      // Enable/Disable DTMF digit logging according to UserAgentMasterProfile setting
//      mMediaInterface->allowLoggingDTMFDigits(userAgent->getUserAgentMasterProfile()->dtmfDigitLoggingEnabled());
//   }

}

ConversationHandle
GstMediaStackAdapter::createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, ConversationManager::AutoHoldMode autoHoldMode)
{
   if (isShuttingDown()) return 0;  // Don't allow new things to be created when we are shutting down

   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(&getConversationManager(), convHandle, autoHoldMode, sharedMediaInterfaceConvHandle);
   post(cmd);
   return convHandle;
}

void
GstMediaStackAdapter::outputBridgeMatrixImpl(ConversationHandle convHandle)
{
   // Note: convHandle of 0 only makes sense if sipXGlobalMediaInterfaceMode is enabled
   if (convHandle == 0)
   {
      if (getBridgeMixer() != 0)
      {
         getBridgeMixer()->outputBridgeMixWeights();
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix request with no conversation handle is not appropriate for current MediaInterfaceMode");
      }
   }
   else
   {
      Conversation* conversation = getConversation(convHandle);
      if (conversation)
      {
         if (conversation->getBridgeMixer() != 0)
         {
            conversation->getBridgeMixer()->outputBridgeMixWeights();
         }
         else
         {
            WarningLog(<< "ConversationManager::outputBridgeMatrix requested conversation wihtout a mixer/media interface, conversationHandle=" << convHandle);
         }
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix requested for non-existing conversationHandle=" << convHandle);
      }
   }
}

void
GstMediaStackAdapter::buildSessionCapabilities(const resip::Data& ipaddress,
                               const std::vector<unsigned int>& codecIds, resip::SdpContents& sessionCaps)
{
   // FIXME VP8
   // FIXME OPUS
   // FIXME Gst:
   // - check who calls this (called from testUA, reConServer, ...
   // - adapt for Gstreamer

   // as a hack, we populate it with a static copy of the SDP from Gstreamer

   // FIXME: ipaddress is assumed to be IP4
   Data caps("v=0\r\n"
             "o=- 3843572881 3843572881 IN IP4 " + ipaddress + "\r\n"
             "s=Gstreamer Media Server\r\n"
             "c=IN IP4 " + ipaddress + "\r\n"
             "t=0 0\r\n"
             "m=audio 34712 RTP/AVP 0\r\n"
             "a=sendrecv\r\n"
             "a=rtcp:34713\r\n"
             "a=rtpmap:0 PCMU/8000\r\n"
             "a=ssrc:866007149 cname:user2141419334@host-99aef493\r\n"
             "m=video 2962 RTP/AVP 97 126\r\n"
             "a=sendrecv\r\n"
             "a=rtcp:2963\r\n"
             "a=rtpmap:97 H264/90000\r\n"
             "a=rtpmap:126 H264/90000\r\n"
             "a=fmtp:97 packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16\r\n"
             "a=fmtp:126 packetization-mode=1;profile-level-id=428016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16\r\n"
             "a=ssrc:1014502683 cname:user2141419334@host-99aef493\r\n");

   HeaderFieldValue hfv(caps.data(), caps.size());
   Mime type("application", "sdp");
   SdpContents sdp(hfv, type);
   sessionCaps = sdp;
}

bool
GstMediaStackAdapter::supportsMultipleMediaInterfaces()
{
   return true;
}

bool
GstMediaStackAdapter::canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2)
{
   return false;
}

Conversation *
GstMediaStackAdapter::createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode)
{
   return new GstConversation(handle, getConversationManager(), *this, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode);
}

LocalParticipant *
GstMediaStackAdapter::createLocalParticipantInstance(ParticipantHandle partHandle)
{
   return 0;
}

MediaResourceParticipant *
GstMediaStackAdapter::createMediaResourceParticipantInstance(ParticipantHandle partHandle, const resip::Uri& mediaUrl, const std::shared_ptr<resip::Data>& audioBuffer, void* recordingCircularBuffer)
{
   return 0; // FIXME Gst - implement MediaResourceParticipant in Gst
}

void
GstMediaStackAdapter::configureRemoteParticipantInstance(GstRemoteParticipant* krp)
{
   DebugLog(<<"not implemented");
}

RemoteParticipant *
GstMediaStackAdapter::createRemoteParticipantInstance(DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   GstRemoteParticipant *rp = new GstRemoteParticipant(getConversationManager(), *this, dum, rpds);
   configureRemoteParticipantInstance(rp);
   return rp;
}

RemoteParticipant *
GstMediaStackAdapter::createRemoteParticipantInstance(ParticipantHandle partHandle, DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   GstRemoteParticipant *rp = new GstRemoteParticipant(partHandle, getConversationManager(), *this, dum, rpds);
   configureRemoteParticipantInstance(rp);
   return rp;
}

RemoteParticipantDialogSet *
GstMediaStackAdapter::createRemoteParticipantDialogSetInstance(
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new GstRemoteParticipantDialogSet(getConversationManager(), *this, forkSelectMode, conversationProfile);
}

void
GstMediaStackAdapter::process()
{
   // Gstreamer has its own thread so we don't need to do anything here
}

void
GstMediaStackAdapter::initializeDtlsFactory(const resip::Data& defaultAoR)
{
   // FIXME Gst
}


/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2007-2008, Plantronics, Inc.
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
