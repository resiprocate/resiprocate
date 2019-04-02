#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include <process.h>
#else
#include <spawn.h>
#endif

#include "../UserAgent.hxx"
#include "Server.hxx"
#include "AppSubsystem.hxx"
#include "WebAdmin.hxx"
#include "WebAdminThread.hxx"

#include <resip/stack/Tuple.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ThreadIf.hxx>
#include <rutil/WinLeakCheck.hxx>

// sipX includes
#include <os/OsSysLog.h>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER

namespace mohparkserver
{

class SdpMessageDecorator : public MessageDecorator
{
public:
   virtual ~SdpMessageDecorator() {}
   virtual void decorateMessage(SipMessage &msg, 
                                const Tuple &source,
                                const Tuple &destination,
                                const Data& sigcompId)
   {
      SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
      if(sdp && sdp->session().media().size() > 0 && sdp->session().connection().getAddress() == "0.0.0.0")  
      {
         // Fill in IP and Port from source
         sdp->session().connection().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);
         sdp->session().origin().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);
         DebugLog( << "SdpMessageDecorator: src=" << source << ", dest=" << destination << ", msg=" << endl << msg.brief());
      }
   }
   virtual void rollbackMessage(SipMessage& msg) {}  // Nothing to do
   virtual MessageDecorator* clone() const { return new SdpMessageDecorator; }
};

class MyUserAgent : public UserAgent
{
public:
   MyUserAgent(Server& server, SharedPtr<UserAgentMasterProfile> profile, resip::AfterSocketCreationFuncPtr socketFunc) :
      UserAgent(&server, profile, socketFunc),
      mServer(server) {}

   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
   {
      if(id == MAXPARKTIMEOUT)
      {
          mServer.onMaxParkTimeout((ParticipantHandle)seq);
      }
      else
      {
         InfoLog(<< "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
      }
      
   }

   virtual void onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
   {
      InfoLog(<< "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
   }

   virtual void onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData)
   {
      InfoLog(<< "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);
   }
private:
   Server& mServer;
};

class MOHParkServerLogger : public ExternalLogger
{
public:
   virtual ~MOHParkServerLogger() {}
   /** return true to also do default logging, false to supress default logging. */
   virtual bool operator()(Log::Level level,
                           const Subsystem& subsystem, 
                           const Data& appName,
                           const char* file,
                           int line,
                           const Data& message,
                           const Data& messageWithHeaders)
   {
      // Log any warnings/errors to the screen and all MOHParkServer logging messages
      if(level <= Log::Warning || subsystem.getSubsystem() == AppSubsystem::MOHPARKSERVER.getSubsystem())
      {
         resipCout << messageWithHeaders << endl;
      }
      return true;
   }
};
MOHParkServerLogger g_MOHParkServerLogger;

Server::Server(ConfigParser& config) : 
   ConversationManager(false /* local audio? */, ConversationManager::sipXConversationMediaInterfaceMode),
   mConfig(config),
   mIsV6Avail(false),
   mMyUserAgent(0),
   mWebAdmin(0),
   mWebAdminThread(0)
{
   // Initialize loggers
   initializeResipLogging(mConfig.mLogFileMaxBytes, mConfig.mLogLevel, mConfig.mLogFilename);
   if(!mConfig.mSipXLogFilename.empty())
   {
      //enableConsoleOutput(TRUE);  // Allow sipX console output
      OsSysLog::initialize(0, "MOHParkServer");
      OsSysLog::setOutputFile(0, mConfig.mSipXLogFilename.c_str()) ;
   }

   InfoLog( << "MOHParkServer settings:");
   ConfigParser::MOHSettingsMap::iterator itMOH = mConfig.mMOHSettingsMap.begin();
   for (; itMOH != mConfig.mMOHSettingsMap.end(); itMOH++)
   {
      InfoLog(<< "  MOH" << itMOH->first << " URI = " << itMOH->second.mUri);
      InfoLog(<< "  MOH" << itMOH->first << " Registration Time = " << itMOH->second.mRegistrationTime);
      InfoLog(<< "  MOH" << itMOH->first << " Outbound Proxy = " << itMOH->second.mOutboundProxy);
      InfoLog(<< "  MOH" << itMOH->first << " Filename URL = " << itMOH->second.mMOHFilenameUrl);
   }
   ConfigParser::ParkSettingsMap::iterator itPark = mConfig.mParkSettingsMap.begin();
   for (; itPark != mConfig.mParkSettingsMap.end(); itPark++)
   {
      InfoLog(<< "  Park" << itPark->first << " URI = " << itPark->second.mUri);
      InfoLog(<< "  Park" << itPark->first << " Registration Time = " << itPark->second.mRegistrationTime);
      InfoLog(<< "  Park" << itPark->first << " Outbound Proxy = " << itPark->second.mOutboundProxy);
      InfoLog(<< "  Park" << itPark->first << " Filename URL = " << itPark->second.mMOHFilenameUrl);
      InfoLog(<< "  Park" << itPark->first << " Orbit Range Start = " << itPark->second.mOrbitRangeStart);
      InfoLog(<< "  Park" << itPark->first << " Number of Orbits = " << itPark->second.mNumOrbits);
      InfoLog(<< "  Park" << itPark->first << " Orbit Registration Time = " << itPark->second.mOrbitRegistrationTime);
      InfoLog(<< "  Park" << itPark->first << " Max Park Time = " << itPark->second.mMaxParkTime);
   }
   InfoLog( << "  Local IP Address = " << mConfig.mAddress);
   InfoLog( << "  Override DNS Servers = " << mConfig.mDnsServers);
   InfoLog( << "  UDP Port = " << mConfig.mUdpPort);
   InfoLog( << "  TCP Port = " << mConfig.mTcpPort);
   InfoLog( << "  TLS Port = " << mConfig.mTlsPort);
   InfoLog( << "  TLS Domain = " << mConfig.mTlsDomain);
   InfoLog( << "  Certificate Path = " << mConfig.mCertificatePath);
   InfoLog( << "  Keepalives = " << (mConfig.mKeepAlives ? "enabled" : "disabled"));
   InfoLog( << "  Outbound Proxy = " << mConfig.mOutboundProxy);
   InfoLog( << "  Media Port Range Start = " << mConfig.mMediaPortRangeStart);
   InfoLog( << "  Media Port Range Size = " << mConfig.mMediaPortRangeSize);
   InfoLog( << "  Log Level = " << mConfig.mLogLevel);

   resip::Data foo;

   if(!mConfig.mAddress.empty())
   {
      // If address is specified in config file, then just use this address only
      Tuple myTuple(mConfig.mAddress, mConfig.mUdpPort, UDP);
      if(myTuple.ipVersion() == V6)
      {
         mIsV6Avail = true;
      }
   }
   else
   {
      // If address in config is empty - query network interface info
      std::list<std::pair<Data,Data> > interfaces = DnsUtil::getInterfaces();
      std::list<std::pair<Data,Data> >::iterator itInt = interfaces.begin();
      for(;itInt != interfaces.end(); itInt++)
      {
         Tuple myTuple(itInt->second, mConfig.mUdpPort, UDP);
         if(myTuple.ipVersion() == V6)
         {
            mIsV6Avail = true;
         }
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   // Setup UserAgentMasterProfile
   //////////////////////////////////////////////////////////////////////////////

   SharedPtr<UserAgentMasterProfile> profile(new UserAgentMasterProfile);

   // Add transports
   try
   {
      if(mConfig.mUdpPort == (unsigned short)-1 
#ifdef USE_SSL
         && mConfig.mTlsPort == (unsigned short)-1 
#endif
         && mConfig.mTcpPort == (unsigned short)-1)
      {
         // Ensure there is at least one transport enabled - if all are disabled, then enable UDP on an OS selected port
         mConfig.mUdpPort = 0;
      }
      if(mConfig.mUdpPort != (unsigned short)-1)
      {
         profile->addTransport(UDP, mConfig.mUdpPort, V4, StunDisabled, mConfig.mAddress);
         if(mIsV6Avail)
         {
            profile->addTransport(UDP, mConfig.mUdpPort, V6, StunDisabled, mConfig.mAddress);
         }
      }
      if(mConfig.mTcpPort != (unsigned short)-1)
      {
         profile->addTransport(TCP, mConfig.mTcpPort, V4, StunDisabled, mConfig.mAddress);
         if(mIsV6Avail)
         {
            profile->addTransport(TCP, mConfig.mTcpPort, V6, StunDisabled, mConfig.mAddress);
         }
      }
#ifdef USE_SSL
      if(mConfig.mTlsPort != (unsigned short)-1)
      {
         profile->addTransport(TLS, mConfig.mTlsPort, V4, StunDisabled, mConfig.mAddress, mConfig.mTlsDomain);
         if(mIsV6Avail)
         {
            profile->addTransport(TLS, mConfig.mTlsPort, V6, StunDisabled, mConfig.mAddress, mConfig.mTlsDomain);
         }
      }
#endif
   }
   catch (BaseException& e)
   {
      std::cerr << "Cannot start a transport, likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      exit(-1);
   }
   profile->certPath() = mConfig.mCertificatePath;

   // DNS Servers
   ParseBuffer pb(mConfig.mDnsServers);
   Data dnsServer;
   while(!mConfig.mDnsServers.empty() && !pb.eof())
   {
      pb.skipWhitespace();
      const char *start = pb.position();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";,");  // allow white space 
      pb.data(dnsServer, start);
      if(DnsUtil::isIpV4Address(dnsServer))
      {
         InfoLog( << "Adding DNS Server: " << dnsServer);
         profile->addAdditionalDnsServer(dnsServer);
      }
      else
      {
         ErrLog( << "Tried to add dns server, but invalid format: " << dnsServer);
      }
      if(!pb.eof())
      {
         pb.skipChar();
      }
   }   

   // Disable Statisitics Manager
   profile->statisticsManagerEnabled() = false;

   if(mConfig.mKeepAlives)
   {
      profile->setKeepAliveTimeForDatagram(30);
      profile->setKeepAliveTimeForStream(180);
   }

   // Support Methods, etc.
   profile->validateContentEnabled() = false;
   profile->validateContentLanguageEnabled() = false;
   profile->validateAcceptEnabled() = false;

   profile->clearSupportedLanguages();
   profile->addSupportedLanguage(Token("en"));  

   profile->clearSupportedMimeTypes();
   profile->addSupportedMimeType(INVITE, Mime("application", "sdp"));
   profile->addSupportedMimeType(INVITE, Mime("multipart", "mixed"));  
   profile->addSupportedMimeType(INVITE, Mime("multipart", "signed"));  
   profile->addSupportedMimeType(INVITE, Mime("multipart", "alternative"));  
   profile->addSupportedMimeType(OPTIONS,Mime("application", "sdp"));
   profile->addSupportedMimeType(OPTIONS,Mime("multipart", "mixed"));  
   profile->addSupportedMimeType(OPTIONS, Mime("multipart", "signed"));  
   profile->addSupportedMimeType(OPTIONS, Mime("multipart", "alternative"));  
   profile->addSupportedMimeType(NOTIFY, Mime("message", "sipfrag"));  

   profile->clearSupportedMethods();
   profile->addSupportedMethod(INVITE);
   profile->addSupportedMethod(ACK);
   profile->addSupportedMethod(CANCEL);
   profile->addSupportedMethod(OPTIONS);
   profile->addSupportedMethod(BYE);
   profile->addSupportedMethod(REFER);    
   profile->addSupportedMethod(NOTIFY);    
   profile->addSupportedMethod(SUBSCRIBE); 

   profile->clearSupportedOptionTags();
   profile->addSupportedOptionTag(Token(Symbols::Replaces));      
   profile->addSupportedOptionTag(Token(Symbols::Timer)); 
   profile->addSupportedOptionTag(Token(Symbols::NoReferSub));
   profile->addSupportedOptionTag(Token(Symbols::AnswerMode));
   profile->addSupportedOptionTag(Token(Symbols::TargetDialog));

   profile->setUacReliableProvisionalMode(MasterProfile::Never);

   profile->clearSupportedSchemes();
   profile->addSupportedScheme("sip");  
#ifdef USE_SSL
   profile->addSupportedScheme("sips");
#endif

   // Have stack add Allow/Supported/Accept headers to INVITE dialog establishment messages
   profile->clearAdvertisedCapabilities(); // Remove Profile Defaults, then add our preferences
   profile->addAdvertisedCapability(Headers::Allow);  
   //profile->addAdvertisedCapability(Headers::AcceptEncoding);  // This can be misleading - it might specify what is expected in response
   profile->addAdvertisedCapability(Headers::AcceptLanguage);  
   profile->addAdvertisedCapability(Headers::Supported);  
   profile->setMethodsParamEnabled(true);

   profile->setUserAgent("MOHParkServer");
   profile->rtpPortRangeMin() = mConfig.mMediaPortRangeStart; 
   profile->rtpPortRangeMax() = mConfig.mMediaPortRangeStart + mConfig.mMediaPortRangeSize-1; 

   // Install Sdp Message Decorator
   SharedPtr<MessageDecorator> outboundDecorator(new SdpMessageDecorator);
   profile->setOutboundDecorator(outboundDecorator);

   mUserAgentMasterProfile = profile;

   // Create UserAgent
   mMyUserAgent = new MyUserAgent(*this, profile, mConfig.mSocketFunc);

   if(mConfig.mHttpPort != 0)
   {
      // Create WebAdmin
      mWebAdmin = new WebAdmin(*this, true /* noWebChallenges */, Data::Empty, Data::Empty, mConfig.mHttpPort, resip::V4);
      mWebAdminThread = new WebAdminThread(*mWebAdmin);
      resip_assert(mWebAdminThread && mWebAdmin);
      mWebAdminThread->run();
   }
}

Server::~Server()
{
   if(mWebAdminThread) 
   {
       mWebAdminThread->shutdown();
       mWebAdminThread->join();
       delete mWebAdminThread;
       mWebAdminThread = 0;
   }
   if(mWebAdmin)
   {
       delete mWebAdmin;
       mWebAdmin = 0;
   }

   shutdown();
   delete mMyUserAgent;

   // Should be deleted in shutdown - but done here also for safety
   MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
   for (; itMOH != mMOHManagerMap.end(); itMOH++)
   {
      delete itMOH->second;
   }

   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      delete itPark->second;
   }
}

void 
Server::initializeResipLogging(unsigned int maxByteCount, const Data& level, const Data& resipFilename)
{
   // Initialize loggers
   Log::setMaxByteCount(maxByteCount); 
   Log::initialize("file", level.c_str(), "", resipFilename.c_str(), &g_MOHParkServerLogger);
}

void
Server::startup()
{
   resip_assert(mMyUserAgent);
   mMyUserAgent->startup();

   // Create an MOHManager for each setting in map and call startup
   resip_assert(mMOHManagerMap.empty());
   ConfigParser::MOHSettingsMap::iterator itMOH = mConfig.mMOHSettingsMap.begin();
   for (; itMOH != mConfig.mMOHSettingsMap.end(); itMOH++)
   {
      MOHManager* mohManager = new MOHManager(*this);
      mMOHManagerMap[itMOH->first] = mohManager;
      mohManager->startup(itMOH->second);
   }

   // Create a ParkManager for each setting in map can call startup
   assert(mParkManagerMap.empty());
   ConfigParser::ParkSettingsMap::iterator itPark = mConfig.mParkSettingsMap.begin();
   for (; itPark != mConfig.mParkSettingsMap.end(); itPark++)
   {
      ParkManager* parkManager = new ParkManager(*this);
      mParkManagerMap[itPark->first] = parkManager;
      parkManager->startup(itPark->second);
   }
}

void 
Server::process(int timeoutMs)
{
   resip_assert(mMyUserAgent);
   mMyUserAgent->process(timeoutMs);
}

void
Server::shutdown()
{
   MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
   for (; itMOH != mMOHManagerMap.end(); itMOH++)
   {
      itMOH->second->shutdown(true /*shuttingDownServer*/);
      delete itMOH->second;
   }
   mMOHManagerMap.clear();

   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      itPark->second->shutdown(true /*shuttingDownServer*/);
      delete itPark->second;
   }
   mParkManagerMap.clear();

   resip_assert(mMyUserAgent);
   mMyUserAgent->shutdown();
   OsSysLog::shutdown();
}

void 
Server::buildSessionCapabilities(resip::SdpContents& sessionCaps)
{
   unsigned int codecIds[] = { SdpCodec::SDP_CODEC_PCMU /* 0 - pcmu */, 
                               SdpCodec::SDP_CODEC_PCMA /* 8 - pcma */, 
                               SdpCodec::SDP_CODEC_SPEEX /* 96 - speex NB 8,000bps */,
                               SdpCodec::SDP_CODEC_SPEEX_15 /* 98 - speex NB 15,000bps */, 
                               SdpCodec::SDP_CODEC_SPEEX_24 /* 99 - speex NB 24,600bps */,
                               SdpCodec::SDP_CODEC_L16_44100_MONO /* PCM 16 bit/sample 44100 samples/sec. */, 
                               SdpCodec::SDP_CODEC_G726_16,
                               SdpCodec::SDP_CODEC_G726_24,
                               SdpCodec::SDP_CODEC_G726_32,
                               SdpCodec::SDP_CODEC_G726_40,
                               SdpCodec::SDP_CODEC_ILBC /* 108 - iLBC */,
                               SdpCodec::SDP_CODEC_ILBC_20MS /* 109 - Internet Low Bit Rate Codec, 20ms (RFC3951) */, 
                               SdpCodec::SDP_CODEC_SPEEX_5 /* 97 - speex NB 5,950bps */,
                               SdpCodec::SDP_CODEC_GSM /* 3 - GSM */,
                               SdpCodec::SDP_CODEC_TONES /* 110 - telephone-event */};
   unsigned int numCodecIds = sizeof(codecIds) / sizeof(codecIds[0]);
   ConversationManager::buildSessionCapabilities(mConfig.mAddress, numCodecIds, codecIds, sessionCaps);
}

void 
Server::getActiveCallsInfo(std::list<ActiveCallInfo>& callInfos)
{
    callInfos.clear();

    // Get Calls from all the MOH Managers
    MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
    for (; itMOH != mMOHManagerMap.end(); itMOH++)
    {
       itMOH->second->getActiveCallsInfo(callInfos);
    }

    // Get Calls from all the MOH Managers
    ParkManagerMap::iterator itPark = mParkManagerMap.begin();
    for (; itPark != mParkManagerMap.end(); itPark++)
    {
       itPark->second->getActiveCallsInfo(callInfos);
    }
}

void 
Server::onConversationDestroyed(ConversationHandle convHandle)
{
   InfoLog(<< "onConversationDestroyed: handle=" << convHandle);
}

void 
Server::onParticipantDestroyed(ParticipantHandle partHandle)
{
   InfoLog(<< "onParticipantDestroyed: handle=" << partHandle);

   // Try each manager until participant is found
   MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
   for (; itMOH != mMOHManagerMap.end(); itMOH++)
   {
      if (itMOH->second->removeParticipant(partHandle))
      {
         return;
      }
   }

   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      if (itPark->second->removeParticipant(partHandle))
      {
         return;
      }
   }
}

void 
Server::onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up)
{
   InfoLog(<< "onDtmfEvent: handle=" << partHandle << " tone=" << dtmf << " dur=" << duration << " up=" << up);
}

void 
Server::onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onIncomingParticipant: handle=" << partHandle << " auto=" << autoAnswer << " msg=" << msg.brief());

   // Try each manager until conversationProfile is found
   MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
   for (; itMOH != mMOHManagerMap.end(); itMOH++)
   {
      if (itMOH->second->isMyProfile(conversationProfile))
      {
         itMOH->second->addParticipant(partHandle, msg.header(h_From).uri(), msg.header(h_From).uri());
         return;
      }
   }

   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      if (itPark->second->isMyProfile(conversationProfile))
      {
         itPark->second->incomingParticipant(partHandle, msg);
         return;
      }
   }

   // Not found anywhere - reject with a 404
   rejectParticipant(partHandle, 404);
}

void 
Server::onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << msg.brief());

   // Try each manager until conversationProfile is found
   MOHManagerMap::iterator itMOH = mMOHManagerMap.begin();
   for (; itMOH != mMOHManagerMap.end(); itMOH++)
   {
      if (itMOH->second->isMyProfile(conversationProfile))
      {
         itMOH->second->addParticipant(partHandle, msg.header(h_ReferTo).uri().getAorAsUri(), msg.header(h_From).uri());
         return;
      }
   }

   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      if (itPark->second->isMyProfile(conversationProfile))
      {
         itPark->second->parkParticipant(partHandle, msg);
         return;
      }
   }

   // Not found anywhere - reject with a 404
   rejectParticipant(partHandle, 404);
}
    
void 
Server::onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantTerminated: handle=" << partHandle);
}
    
void 
Server::onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantProceeding: handle=" << partHandle << " msg=" << msg.brief());
}

void 
Server::onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                      ConversationHandle origConvHandle, ParticipantHandle origPartHandle)
{
   InfoLog(<< "onRelatedConversation: relatedConvHandle=" << relatedConvHandle << " relatedPartHandle=" << relatedPartHandle
           << " origConvHandle=" << origConvHandle << " origPartHandle=" << origPartHandle);
}

void 
Server::onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantAlerting: handle=" << partHandle << " msg=" << msg.brief());
}
    
void 
Server::onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnected: handle=" << partHandle << " msg=" << msg.brief());
}

void 
Server::onParticipantRedirectSuccess(ParticipantHandle partHandle)
{
   InfoLog(<< "onParticipantRedirectSuccess: handle=" << partHandle);
   destroyParticipant(partHandle);  // Transfer is successful - end participant
}

void 
Server::onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantRedirectFailure: handle=" << partHandle << " statusCode=" << statusCode);
}

void
Server::onParticipantRequestedHold(ParticipantHandle partHandle, bool held)
{
   InfoLog(<< "onParticipantRequestedHold: handle=" << partHandle << " held=" << held);
}

void 
Server::onMaxParkTimeout(recon::ParticipantHandle participantHandle)
{
   // Pass to ParkManager(s) to see if participant is still around
   ParkManagerMap::iterator itPark = mParkManagerMap.begin();
   for (; itPark != mParkManagerMap.end(); itPark++)
   {
      if (itPark->second->onMaxParkTimeout(participantHandle))
      {
         // Once participant is found, we can stop
         return;
      }
   }
}

}

/* ====================================================================

 Copyright (c) 2010-2016, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

