#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"

#include <signal.h>
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include <rutil/WinLeakCheck.hxx>

#ifdef WIN32
extern int sdpTests(void);
#endif 

///////////////////////////////////////////////////////////////////////////
// SCENARIOS UNDER TEST
//
// Scenario 1: Simple call:  Alice -> Bob
//
// Alice                      Bob                      Callback Sequence
// -------------------------- ------------------------ ------------------
// createConversation
// createLocalParticipant
// addParticipant
// createRemoteParticipant
//                            <-onIncomingCall                 1
//                            alertParticpant
// <-onParticipantAlerting                                     2
//                            createConversation
//                            addParticipant
//                            answerParticipant
// <-onParticpantConnected                                     3
// destroyConversation
// <-onParticipantTerminated                                   4
//                            <-onParticipantTerminated        5
//
//
// Scenario 2: Call Rejection:  Bob -> Alice
//
// Alice                      Bob                      Callback Sequence
// -------------------------- ------------------------ ------------------
//                            createConversation
//                            createRemoteParticipant
// <-onIncomingCall                                            1
// rejectParticpant
// <-onParticipantTerminated                                   2
//                            <-onParticipantTerminated        3
//
//
// Scenario 3: Call Redirection:  Bob -> Alice
//
// Alice                      Bob                      Callback Sequence
// -------------------------- ------------------------ ------------------
//                            createConversation
//                            createRemoteParticipant
// <-onIncomingCall                                            1
// redirectParticpant
// <-onParticipantTerminated                                   2
// <-onIncomingCall                                            3
// rejectParticpant
// <-onParticipantTerminated                                   4
//                            <-onParticipantTerminated        5
//
//
// Scenario 4: Call Hold/Transfer:  Bob -> Alice
//
// Alice                      Bob                      Callback Sequence
// -------------------------- ------------------------ ------------------
//                            createConversation
//                            createLocalPartcipant
//                            addPartcipant
//                            createRemoteParticipant
// <-onIncomingCall                                            1
// createConversation
// addParticipant
// alertParticpant(early)
//                            <-onParticipantAlerting          2
// answerParticipant
//                            <-onParticpantConnected          3
//                            removeParticipant(local/hold)    
//                            redirectParticipant to bad dest
//                    call failure (408)
//                           <-onParticipantRedirectFailure    4
//                           destroyParticipant
//                           <-onParticipantTerminated         5
// <-onParticipantTerminated                                   6
//
///////////////////////////////////////////////////////////////////////////

using namespace recon;
using namespace resip;
using namespace std;

unsigned int SCENARIO = 1;
unsigned int LAST_SCENARIO = 4;
unsigned int CALLBACK_SEQUENCE = 1;

NameAddr aliceUri("sip:alice@127.0.0.1:32543");
NameAddr bobUri("sip:bob@127.0.0.1:32544");

//#define LOG_LEVEL resip::Log::Info
#define LOG_LEVEL resip::Log::Warning
#define MARKER  cout << "**** SCENARIO " << SCENARIO << " CALLBACK_SEQUENCE " << CALLBACK_SEQUENCE-1 << " passed! ****" << endl

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

///////////////////////////////////////////////////////////////////////////////
//  ALICE
///////////////////////////////////////////////////////////////////////////////
class AliceConversationManager : public ConversationManager
{
public:
   AliceConversationManager(ConversationManager::MediaInterfaceMode mode) : ConversationManager(true, mode)
   { 
      mLogPrefix = "Alice: ";
   };

   virtual ConversationHandle createConversation()
   {
      ConversationHandle convHandle = ConversationManager::createConversation();
      mConvHandles.push_back(convHandle);
      return convHandle;
   }

   virtual void startup()
   {
      ConversationHandle convHandle = createConversation();    
      mLocalParticipant = createLocalParticipant();
      addParticipant(convHandle, mLocalParticipant);
      createRemoteParticipant(convHandle, bobUri, ConversationManager::ForkSelectAutomatic);
   }

   virtual void onConversationDestroyed(ConversationHandle convHandle)
   {
      InfoLog(<< mLogPrefix << "onConversationDestroyed: handle=" << convHandle);
   }

   virtual void onParticipantDestroyed(ParticipantHandle partHandle)
   {
      InfoLog(<< mLogPrefix << "onParticipantDestroyed: handle=" << partHandle);
   }

   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
   {
      InfoLog(<< mLogPrefix << "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
   }

   virtual void onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
   {
      InfoLog(<< mLogPrefix << "onIncomingParticipant: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 2:
         assert(CALLBACK_SEQUENCE++ == 1); 
         assert(partHandle == 3);    
         assert(autoAnswer == false);
         MARKER;
         rejectParticipant(partHandle, 486);
         break;
      case 3:
         assert(CALLBACK_SEQUENCE == 1 || CALLBACK_SEQUENCE == 3);
         CALLBACK_SEQUENCE++;
         if(CALLBACK_SEQUENCE-1 == 1)
         {
            assert(partHandle == 4);    
            assert(autoAnswer == false);
            MARKER;
            redirectParticipant(partHandle, aliceUri);
         }
         else
         {
            assert(partHandle == 5);    
            assert(autoAnswer == false);
            MARKER;
            rejectParticipant(partHandle, 404);
         }
         break;
      case 4:
         assert(CALLBACK_SEQUENCE++ == 1); 
         assert(partHandle == 6);    
         assert(autoAnswer == false);
         MARKER;
         {
         ConversationHandle convHandle = createConversation();
         addParticipant(convHandle, partHandle);
         }
         alertParticipant(partHandle, true);
         answerParticipant(partHandle);
         break;
      default:
         assert(false);
         break;
      }
   }

   virtual void onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile)
   {
      InfoLog(<< mLogPrefix << "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << msg.brief());
   }

   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
   {
      InfoLog(<< mLogPrefix << "onParticipantTerminated: handle=" << partHandle << " status=" << statusCode);

      switch(SCENARIO)
      {
      case 1:
         assert(CALLBACK_SEQUENCE++ == 4);
         assert(partHandle == 2);    // Note:  LocalParticipant is part handle 1, remote is 2
         assert(statusCode == 0);
         MARKER;
         break;
      case 2:
         assert(CALLBACK_SEQUENCE++ == 2);
         assert(partHandle == 3);    
         assert(statusCode == 0);
         MARKER;
         break;
      case 3:
         assert(CALLBACK_SEQUENCE == 2 || CALLBACK_SEQUENCE == 4);
         CALLBACK_SEQUENCE++;
         if(CALLBACK_SEQUENCE-1 == 2)
         {
            assert(partHandle == 4);    
            assert(statusCode == 0);
            MARKER;
         }
         else
         {
            assert(partHandle == 5);    
            assert(statusCode == 0);
            MARKER;
         }
         break;
      case 4:
         assert(CALLBACK_SEQUENCE++ == 6);
         assert(partHandle == 6);    
         assert(statusCode == 0);
         MARKER;
         break;
      default:
         assert(false);
         break;
      }

      // Destroy all conversations
      std::list<ConversationHandle>::iterator it;
      for(it = mConvHandles.begin(); it != mConvHandles.end(); it++)
      {
         destroyConversation(*it);
      }
      mConvHandles.clear();
   }
    
   virtual void onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantProceeding: handle=" << partHandle << " msg=" << msg.brief());
   }

   virtual void onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                      ConversationHandle origConvHandle, ParticipantHandle origPartHandle)
   {
      InfoLog(<< mLogPrefix << "onRelatedConversation: relatedConvHandle=" << relatedConvHandle << " relatedPartHandle=" << relatedPartHandle
              << " origConvHandle=" << origConvHandle << " origPartHandle=" << origPartHandle);
      mConvHandles.push_back(relatedConvHandle);
   }

   virtual void onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantAlerting: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 1:
         assert(CALLBACK_SEQUENCE++ == 2);
         assert(partHandle == 2);    // Note:  LocalParticipant is part handle 1, remote is 2
         MARKER;
         break;
      default:
         assert(false);
         break;
      }
   }
    
   virtual void onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantConnected: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 1:
         assert(CALLBACK_SEQUENCE++ == 3);
         assert(partHandle == 2);    // Note:  LocalParticipant is part handle 1, remote is 2
         MARKER;
         {
         // Destroy all conversations
         std::list<ConversationHandle>::iterator it;
         for(it = mConvHandles.begin(); it != mConvHandles.end(); it++)
         {
            destroyConversation(*it);
         }
         mConvHandles.clear();
         }
         break;
      default:
         assert(false);
         break;
      }
   }

   virtual void onParticipantRedirectSuccess(ParticipantHandle partHandle)
   {
      InfoLog(<< mLogPrefix << "onParticipantRedirectSuccess: handle=" << partHandle);
   }

   virtual void onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode)
   {
      InfoLog(<< mLogPrefix << "onParticipantRedirectFailure: handle=" << partHandle << " statusCode=" << statusCode);
   }

   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up) {}

private:
   std::list<ConversationHandle> mConvHandles;
   ParticipantHandle mLocalParticipant;
   Data mLogPrefix;
};

///////////////////////////////////////////////////////////////////////////////
//  BOB
///////////////////////////////////////////////////////////////////////////////
class BobConversationManager : public ConversationManager
{
public:
   BobConversationManager(ConversationManager::MediaInterfaceMode mode) : ConversationManager(true, mode)
   { 
      mLogPrefix = "Bob: ";
   };

   virtual ConversationHandle createConversation()
   {
      ConversationHandle convHandle = ConversationManager::createConversation();
      mConvHandles.push_back(convHandle);
      return convHandle;
   }

   virtual void startup()
   {
   }

   virtual void onConversationDestroyed(ConversationHandle convHandle)
   {
      InfoLog(<< mLogPrefix << "onConversationDestroyed: handle=" << convHandle);
   }

   virtual void onParticipantDestroyed(ParticipantHandle partHandle)
   {
      InfoLog(<< mLogPrefix << "onParticipantDestroyed: handle=" << partHandle);
   }

   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
   {
      InfoLog(<< mLogPrefix << "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);

      if(SCENARIO == LAST_SCENARIO)
      {
         // End Test
         finished = true;
      }
      else
      {
         // Kick off next scenario
         cout << endl;
         SCENARIO++;
         CALLBACK_SEQUENCE = 1;
         switch(SCENARIO)
         {
         case 2:
         case 3:
            {
               ConversationHandle convHandle = createConversation();    
               createRemoteParticipant(convHandle, aliceUri, ConversationManager::ForkSelectAutomatic);
            }
            break;
         case 4:
            {
               ConversationHandle convHandle = createConversation();    
               mLocalParticipant = createLocalParticipant();
               addParticipant(convHandle, mLocalParticipant);
               createRemoteParticipant(convHandle, aliceUri, ConversationManager::ForkSelectAutomatic);
            }
            break;
         default:
            assert(false);
            break;
         }
      }
   }

   virtual void onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
   {
      InfoLog(<< mLogPrefix << "onIncomingParticipant: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 1:
         assert(CALLBACK_SEQUENCE++ == 1);
         assert(partHandle == 1);    // Note:  Bob has no local participant
         assert(autoAnswer == false);
         MARKER;
         alertParticipant(partHandle, false);
         {
            ConversationHandle convHandle = createConversation();
            addParticipant(convHandle, partHandle);
         }
         answerParticipant(partHandle);
         break;
      default:
         assert(false);
         break;
      }
   }

   virtual void onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile)
   {
      InfoLog(<< mLogPrefix << "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << msg.brief());
      if(mConvHandles.empty())
      {
         ConversationHandle convHandle = createConversation();
         addParticipant(convHandle, partHandle);
      }
   }

   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
   {
      InfoLog(<< mLogPrefix << "onParticipantTerminated: handle=" << partHandle << " status=" << statusCode);

      switch(SCENARIO)
      {
      case 1:
         assert(CALLBACK_SEQUENCE++ == 5);
         assert(partHandle == 1);    // Note:  Bob has no local participant
         assert(statusCode == 0);
         MARKER;

         // Next Scenario
         getUserAgent()->startApplicationTimer(0, 500, 0);
         break;
      case 2:
         assert(CALLBACK_SEQUENCE++ == 3);
         assert(partHandle == 2);    
         assert(statusCode == 486);
         MARKER;

         // End Test
         getUserAgent()->startApplicationTimer(0, 500, 0);
         break;
      case 3:
         assert(CALLBACK_SEQUENCE++ == 5);
         assert(partHandle == 3);    
         assert(statusCode == 404);
         MARKER;

         // End Test
         getUserAgent()->startApplicationTimer(0, 500, 0);
         break;
      case 4:
         assert(CALLBACK_SEQUENCE++ == 5);
         assert(partHandle == 5);    
         assert(statusCode == 0);
         MARKER;

         // End Test
         getUserAgent()->startApplicationTimer(0, 500, 0);
         break;
      default:
         assert(false);
         break;
      }

      // Destroy all conversations
      std::list<ConversationHandle>::iterator it;
      for(it = mConvHandles.begin(); it != mConvHandles.end(); it++)
      {
         destroyConversation(*it);
      }
      mConvHandles.clear();
   }
    
   virtual void onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantProceeding: handle=" << partHandle << " msg=" << msg.brief());
   }

   virtual void onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                      ConversationHandle origConvHandle, ParticipantHandle origPartHandle)
   {
      InfoLog(<< mLogPrefix << "onRelatedConversation: relatedConvHandle=" << relatedConvHandle << " relatedPartHandle=" << relatedPartHandle
              << " origConvHandle=" << origConvHandle << " origPartHandle=" << origPartHandle);
      mConvHandles.push_back(relatedConvHandle);
   }

   virtual void onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantAlerting: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 4:
         assert(CALLBACK_SEQUENCE++ == 2);
         assert(partHandle == 5); 
         MARKER;
         break;
      default:
         assert(false);
         break;
      }
   }
    
   virtual void onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg)
   {
      InfoLog(<< mLogPrefix << "onParticipantConnected: handle=" << partHandle << " msg=" << msg.brief());
      switch(SCENARIO)
      {
      case 4:
         assert(CALLBACK_SEQUENCE++ == 3);
         assert(partHandle == 5); 
         MARKER;
         removeParticipant(mConvHandles.front(), mLocalParticipant);   // hold
         {
            NameAddr badAddress("sip:badAddress@127.0.0.1:33333");
            redirectParticipant(partHandle, badAddress);
         }
         break;
      default:
         assert(false);
         break;
      }
   }

   virtual void onParticipantRedirectSuccess(ParticipantHandle partHandle)
   {
      InfoLog(<< mLogPrefix << "onParticipantRedirectSuccess: handle=" << partHandle);
   }

   virtual void onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode)
   {
      InfoLog(<< mLogPrefix << "onParticipantRedirectFailure: handle=" << partHandle << " statusCode=" << statusCode);
      switch(SCENARIO)
      {
      case 4:
         assert(CALLBACK_SEQUENCE++ == 4);
         assert(partHandle == 5); 
         MARKER;
         destroyParticipant(partHandle);
         break;
      default:
         assert(false);
         break;
      }
   }

   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up) {}

private:
   std::list<ConversationHandle> mConvHandles;
   ParticipantHandle mLocalParticipant;
   Data mLogPrefix;
};


class MyUserAgent : public UserAgent
{
public:
   MyUserAgent(ConversationManager* conversationManager, SharedPtr<UserAgentMasterProfile> profile) :
      UserAgent(conversationManager, profile) {}

   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
   {
      //InfoLog(<< "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
      BobConversationManager* bcm = dynamic_cast<BobConversationManager*>(getConversationManager());
      if(bcm)
      {
         bcm->onApplicationTimer(id, durationMs, seq);
      }
      else
      {
         AliceConversationManager* acm = dynamic_cast<AliceConversationManager*>(getConversationManager());
         if(acm)
         {
            acm->onApplicationTimer(id, durationMs, seq);
         }
      }
   }

   virtual void onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
   {
      InfoLog(<< "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
   }

   virtual void onSubscriptionNotify(SubscriptionHandle handle, Data& notifyData)
   {
      InfoLog(<< "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);
   }
};


SharedPtr<UserAgentMasterProfile> createUserAgentMasterProfile()
{
   SharedPtr<UserAgentMasterProfile> profile(new UserAgentMasterProfile);

   // Settings
   profile->statisticsManagerEnabled() = false;
   profile->validateContentEnabled() = false;
   profile->validateContentLanguageEnabled() = false;
   profile->validateAcceptEnabled() = false;
   profile->clearSupportedLanguages();
   profile->addSupportedLanguage(Token("en"));  
   profile->clearSupportedMimeTypes();
   profile->addSupportedMimeType(INVITE, Mime("application", "sdp"));
   profile->addSupportedMimeType(OPTIONS,Mime("application", "sdp"));
   profile->addSupportedMimeType(UPDATE, Mime("application", "sdp"));  
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
   profile->addSupportedMethod(UPDATE);    
   profile->clearSupportedOptionTags();
   profile->addSupportedOptionTag(Token(Symbols::Replaces));      
   profile->addSupportedOptionTag(Token(Symbols::Timer)); 
   profile->addSupportedOptionTag(Token(Symbols::NoReferSub));
   profile->addSupportedOptionTag(Token(Symbols::AnswerMode));
   profile->addSupportedOptionTag(Token(Symbols::TargetDialog));
   //profile->addSupportedOptionTag(Token(Symbols::C100rel));  
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

   return profile;
}

SharedPtr<ConversationProfile> createConversationProfile(SharedPtr<UserAgentMasterProfile> profile, int port)
{
   SharedPtr<ConversationProfile> conversationProfile(new ConversationProfile(profile));
   conversationProfile->setDefaultRegistrationTime(0);

   // Create Session Capabilities and assign to coversation Profile
   Data address("127.0.0.1");
	// Build s=, o=, t=, and c= lines
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, SdpContents::IP4, address);   // o=   Note:  sessionId and version will be replace in actual offer/answer
	SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, address);  // c=
	session.addTime(SdpContents::Session::Time(0, 0));
	// Build Codecs and media offering
	SdpContents::Session::Medium medium("audio", port, 0, "RTP/AVP");
	SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;  /* RFC3551 */ ;
	medium.addCodec(g711ucodec);
   medium.addAttribute("ptime", Data(20));  // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
   medium.addAttribute("sendrecv");
	session.addMedium(medium);

   SdpContents sessionCaps;   
	sessionCaps.session() = session;
   conversationProfile->sessionCaps() = sessionCaps;

   return conversationProfile;
}

void executeConversationTest(ConversationManager::MediaInterfaceMode mode)
{
   //////////////////////////////////////////////////////////////////////////////
   // Setup UserAgentMasterProfiles
   //////////////////////////////////////////////////////////////////////////////
   SharedPtr<UserAgentMasterProfile> profileAlice = createUserAgentMasterProfile();
   profileAlice->addTransport(UDP, aliceUri.uri().port(), V4);
   SharedPtr<ConversationProfile> conversationProfileAlice = createConversationProfile(profileAlice, 16384);
   conversationProfileAlice->setDefaultFrom(aliceUri);
   conversationProfileAlice->setUserAgent("Test-Alice");

   SharedPtr<UserAgentMasterProfile> profileBob = createUserAgentMasterProfile();
   profileBob->addTransport(UDP, bobUri.uri().port(), V4);
   SharedPtr<ConversationProfile> conversationProfileBob = createConversationProfile(profileBob, 16385);
   conversationProfileBob->setDefaultFrom(bobUri);
   conversationProfileBob->setUserAgent("Test-Bob");

   InfoLog(<< "Tests for sipXGlobalMediaInterfaceMode");

   //////////////////////////////////////////////////////////////////////////////
   // Create ConverationManagers and UserAgents
   //////////////////////////////////////////////////////////////////////////////

   {
      AliceConversationManager aliceConversationManager(mode);
      MyUserAgent aliceUa(&aliceConversationManager, profileAlice);
      aliceUa.addConversationProfile(conversationProfileAlice);

      BobConversationManager   bobConversationManager(mode);
      MyUserAgent bobUa(&bobConversationManager, profileBob);
      bobUa.addConversationProfile(conversationProfileBob);

      //////////////////////////////////////////////////////////////////////////////
      // Startup and run...
      //////////////////////////////////////////////////////////////////////////////

      aliceUa.startup();
      aliceConversationManager.startup();

      bobUa.startup();
      bobConversationManager.startup();

      while(true)
      {
         aliceUa.process(50);
         bobUa.process(50);
         if(finished) break;
      }

      aliceUa.shutdown();
      bobUa.shutdown();
   }
}

int 
main (int argc, char** argv)
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
   {
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

#ifdef WIN32
   // Run SdpTests
   sdpTests();
#endif

   Log::initialize(Log::Cout, LOG_LEVEL, argv[0]);
   //Log::initialize(Log::Cout, resip::Log::Debug, argv[0]);
   initNetwork();

   cout << "Tests for sipXConversationMediaInterfaceMode" << endl;
   executeConversationTest(ConversationManager::sipXConversationMediaInterfaceMode);

   // Reset counters, etc.
   SCENARIO = 1;
   LAST_SCENARIO = 4;
   CALLBACK_SEQUENCE = 1;
   finished = false;

   cout << "Tests for sipXGlobalMediaInterfaceMode" << endl;
   executeConversationTest(ConversationManager::sipXGlobalMediaInterfaceMode);

   InfoLog(<< "unitTests is shutdown.");
   //sleepSeconds(10);

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }  // End FML scope
#endif

}



/* ====================================================================

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
