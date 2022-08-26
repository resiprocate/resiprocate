#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#ifdef WIN32
#include <conio.h>
#else
/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#ifndef __GNUC__
 #include <stropts.h>
#endif
#include <sys/ioctl.h>

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif

#include "../SipXConversationManager.hxx"
#include "../UserAgent.hxx"
#include "../ReconSubsystem.hxx"

#include <os/OsSysLog.h>

// Test Prompts for cache testing
#include "playback_prompt.h"
#include "record_prompt.h"

#include <resip/stack/PlainContents.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <rutil/Time.hxx>
#include <rutil/WinLeakCheck.hxx>

#include <resip/recon/LocalParticipant.hxx>
#include <resip/recon/RemoteIMPagerParticipant.hxx>
#include <resip/recon/RemoteIMSessionParticipant.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

//#define TESTUA_LOGFULLMESSAGES
#ifdef TESTUA_LOGFULLMESSAGES
  #define LOG_PREFIX << "[@@@@@@@@@@@]"    // Make it easier to see testUA output in console
  #define OUTPUTMSG msg
#else
  #define LOG_PREFIX
  #define OUTPUTMSG msg.brief()
#endif


static bool finished = false;
NameAddr uri("sip:noreg@127.0.0.1");
bool autoAnswerEnabled = false;  // If enabled then testUA will automatically answer incoming calls by adding to lowest numbered conversation
std::shared_ptr<ConversationProfile> g_conversationProfile;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

class MyUserAgent : public UserAgent
{
public:
   MyUserAgent(ConversationManager* conversationManager, std::shared_ptr<UserAgentMasterProfile> profile) :
      UserAgent(conversationManager, std::move(profile)) {}
    
   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq) override
   {
      InfoLog(LOG_PREFIX << "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
   }

   virtual void onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode) override
   {
      InfoLog(LOG_PREFIX << "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
   }

   virtual void onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData) override
   {
      InfoLog(LOG_PREFIX << "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);
   }
};

class MyConversationManager : public ConversationManager
{
public:

   MyConversationManager(bool localAudioEnabled, bool multipleMediaInterfaces, bool defaultAutoHoldModeToDisabled)
      : ConversationManager(nullptr),
        mLocalAudioEnabled(localAudioEnabled)
   {
      setMediaStackAdapter(make_shared<SipXConversationManager>(*this, localAudioEnabled, multipleMediaInterfaces ? SipXConversationManager::MediaInterfaceMode::sipXConversationMediaInterfaceMode : SipXConversationManager::MediaInterfaceMode::sipXGlobalMediaInterfaceMode));
      mDefaultAutoHoldMode = defaultAutoHoldModeToDisabled ? ConversationManager::AutoHoldDisabled : ConversationManager::AutoHoldEnabled;
   }

   virtual void startup()
   {      
      if(mLocalAudioEnabled)
      {
         // Create initial local participant and conversation  
         ConversationHandle initialConversation = createConversation(mDefaultAutoHoldMode);
         addParticipant(initialConversation, createLocalParticipant());
         resip::Uri uri("tone:dialtone;duration=1000");
         createMediaResourceParticipant(initialConversation, uri);
      }
      else
      {
         // If no local audio - just create a starter conversation
         createConversation(mDefaultAutoHoldMode);
      }
   
      // Load 2 items into cache for testing
      {
         resip::Data buffer(Data::Share, (const char*)playback_prompt, sizeof(playback_prompt));
         resip::Data name("playback");
         addBufferToMediaResourceCache(name, buffer, 0);
      }
      {
         resip::Data buffer(Data::Share, (const char *)record_prompt, sizeof(record_prompt));
         resip::Data name("record");
         addBufferToMediaResourceCache(name, buffer, 0);
      }      
   }

   
   virtual void onRemoteParticipantConstructed(RemoteParticipant *rp) override
   {
   }


   virtual void onConversationDestroyed(ConversationHandle convHandle) override
   {
      InfoLog(LOG_PREFIX << "onConversationDestroyed: handle=" << convHandle);
   }

   virtual void onParticipantDestroyed(ParticipantHandle partHandle) override
   {
      InfoLog(LOG_PREFIX << "onParticipantDestroyed: handle=" << partHandle);
   }

   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up) override
   {
      InfoLog(LOG_PREFIX << "onDtmfEvent: handle=" << partHandle << " tone=" << dtmf << " dur=" << duration << " up=" << up);
   }

   virtual void onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile) override
   {
      InfoLog(LOG_PREFIX << "onIncomingParticipant: handle=" << partHandle << "auto=" << autoAnswer << " msg=" << OUTPUTMSG);
      if(autoAnswerEnabled)
      {
         // If there are no conversations, then create one, otherwise use first in set
         ConversationHandle convHandle = 0;
         const set<ConversationHandle> conversationHandles = getConversationHandles();
         if (getConversationHandles().empty())
         {
            convHandle = createConversation(mDefaultAutoHoldMode);
         }
         else
         {
            convHandle = *conversationHandles.begin();
         }

         if (mLocalAudioEnabled)
         {
            ParticipantHandle localPartHandle = 0;
            const set<ParticipantHandle> participantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Local);
            // If no local participant then create one, otherwise use first in set
            if (participantHandles.empty())
            {
               localPartHandle = createLocalParticipant();
            }
            else
            {
               localPartHandle = *participantHandles.begin();
            } 
            // Add local participant to conversation
            addParticipant(convHandle, localPartHandle);
         }

         // Add new incoming participant to conversation, and answer call
         addParticipant(convHandle, partHandle);
         answerParticipant(partHandle);
      }
   }

   virtual void onIncomingIMPagerParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, ConversationProfile& conversationProfile) override
   {
      InfoLog(LOG_PREFIX << "onIncomingIMPagerParticipant: handle=" << partHandle << " msg=" << OUTPUTMSG);
      if (autoAnswerEnabled)
      {
         answerParticipant(partHandle);  // sends a 200 OK response
      }
   }

   virtual void onIncomingIMSessionParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile) override
   {
      InfoLog(LOG_PREFIX << "onIncomingIMSessionParticipant: handle=" << partHandle << "auto=" << autoAnswer << " msg=" << OUTPUTMSG);
      if (autoAnswerEnabled)
      {
         answerParticipant(partHandle);  // sends a 200 OK response
      }
   }

   virtual bool onReceiveIMFromParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg) override
   {
      InfoLog(LOG_PREFIX << "onReceiveIMFromParticipant: handle=" << partHandle << " msg=" << OUTPUTMSG);
      PlainContents* plainContents = dynamic_cast<PlainContents*>(msg.getContents());
      if (plainContents)
      {
         InfoLog(LOG_PREFIX << "onReceiveIMFromParticipant: handle=" << partHandle << " IM Text=" << plainContents->text());
      }
      return true;
   }
   
   virtual void onParticipantSendIMFailure(ParticipantHandle partHandle, const SipMessage& msg, std::unique_ptr<Contents> contents) override
   {
      InfoLog(LOG_PREFIX << "onParticipantSendIMFailure: handle=" << partHandle << " msg=" << OUTPUTMSG);
   }

   virtual void onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile) override
   {
      InfoLog(LOG_PREFIX << "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << OUTPUTMSG);
      /*
      if(mConvHandles.empty())
      {
         ConversationHandle convHandle = createConversation();
         addParticipant(convHandle, partHandle);
      }*/
   }
    
   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode) override
   {
      InfoLog(LOG_PREFIX << "onParticipantTerminated: handle=" << partHandle << ", statusCode=" << statusCode);
   }
    
   virtual void onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg) override
   {
      InfoLog(LOG_PREFIX << "onParticipantProceeding: handle=" << partHandle << " msg=" << OUTPUTMSG);
   }

   virtual void onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                      ConversationHandle origConvHandle, ParticipantHandle origPartHandle) override
   {
      InfoLog(LOG_PREFIX << "onRelatedConversation: relatedConvHandle=" << relatedConvHandle << " relatedPartHandle=" << relatedPartHandle
              << " origConvHandle=" << origConvHandle << " origPartHandle=" << origPartHandle);
   }

   virtual void onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg) override
   {
      InfoLog(LOG_PREFIX << "onParticipantAlerting: handle=" << partHandle << " msg=" << OUTPUTMSG);
   }
    
   virtual void onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg) override
   {
      InfoLog(LOG_PREFIX << "onParticipantConnected: handle=" << partHandle << " msg=" << OUTPUTMSG);
   }

   virtual void onParticipantRedirectSuccess(ParticipantHandle partHandle) override
   {
      InfoLog(LOG_PREFIX << "onParticipantRedirectSuccess: handle=" << partHandle);
   }

   virtual void onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode) override
   {
      InfoLog(LOG_PREFIX << "onParticipantRedirectFailure: handle=" << partHandle << " statusCode=" << statusCode);
   }

   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held) override
   {
      InfoLog(LOG_PREFIX << "onParticipantRequestedHold: handle=" << partHandle << " held=" << held);
   }

   virtual void onApplicationTimer(unsigned int timerId, unsigned int sequenceId, unsigned int timerData2) override
   {
      InfoLog(LOG_PREFIX << "onApplicationTimer: timerId=" << timerId << " sequenceId=" << sequenceId);
   }

   void displayInfo()
   {
      Data output;

      const set<ConversationHandle> conversations = getConversationHandles();
      if(!conversations.empty())
      {
         output = "Active conversation handles: ";
         set<ConversationHandle>::const_iterator it;
         for(it = conversations.begin(); it != conversations.end(); it++)
         {
            output += Data(*it) + " ";
         }
         InfoLog(LOG_PREFIX << output);
      }
      const set<ParticipantHandle> localParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Local);
      if(!localParticipantHandles.empty())
      {
         output = "Local Participant handles: ";
         std::set<ParticipantHandle>::iterator it;
         for(it = localParticipantHandles.begin(); it != localParticipantHandles.end(); it++)
         {
            output += Data(*it) + " ";
         }
         InfoLog(LOG_PREFIX << output);
      }
      const set<ParticipantHandle> remoteParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Remote);
      if(!remoteParticipantHandles.empty())
      {
         output = "Remote Participant handles: ";
         std::set<ParticipantHandle>::iterator it;
         for(it = remoteParticipantHandles.begin(); it != remoteParticipantHandles.end(); it++)
         {
            output += Data(*it) + " ";
         }
         InfoLog(LOG_PREFIX << output);
      }
      set<ParticipantHandle> remoteIMParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_RemoteIMPager);
      const set<ParticipantHandle> remoteIMSessionParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_RemoteIMSession);
      remoteIMParticipantHandles.insert(remoteIMSessionParticipantHandles.begin(), remoteIMSessionParticipantHandles.end());  // merge the two lists
      if (!remoteIMParticipantHandles.empty())
      {
         output = "Remote IM Participant handles: ";
         std::set<ParticipantHandle>::iterator it;
         for (it = remoteIMParticipantHandles.begin(); it != remoteIMParticipantHandles.end(); it++)
         {
            output += Data(*it) + " ";
         }
         InfoLog(LOG_PREFIX << output);
      }
      const set<ParticipantHandle> mediaParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_MediaResource);
      if(!mediaParticipantHandles.empty())
      {
         output = "Media Participant handles: ";
         std::set<ParticipantHandle>::iterator it;
         for(it = mediaParticipantHandles.begin(); it != mediaParticipantHandles.end(); it++)
         {
            output += Data(*it) + " ";
         }
         InfoLog(LOG_PREFIX << output);
      }
   }

   bool mLocalAudioEnabled;
   ConversationManager::AutoHoldMode mDefaultAutoHoldMode;
};

void processCommandLine(Data& commandline, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent)
{
   Data command;
#define MAX_ARGS 5
   Data arg[MAX_ARGS];
   ParseBuffer pb(commandline);
   pb.skipWhitespace();
   if(pb.eof()) return;
   const char *start = pb.position();
   pb.skipToOneOf(ParseBuffer::Whitespace);
   pb.data(command, start);

   // Get arguments (up to MAX_ARGS)
   int currentArg = 0;
   while(!pb.eof() && currentArg < MAX_ARGS)
   {
      pb.skipWhitespace();
      if(!pb.eof())
      {
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace);
         pb.data(arg[currentArg++], start);
      }
   }

   SipXConversationManager& mediaStackAdapter = static_cast<SipXConversationManager&>(myConversationManager.getMediaStackAdapter());

   // Process commands
   if(isEqualNoCase(command, "quit") || isEqualNoCase(command, "q") || isEqualNoCase(command, "exit") || isEqualNoCase(command, "x"))
   {
      finished=true;
      return;
   }   
   if(isEqualNoCase(command, "createconv") || isEqualNoCase(command, "cc"))
   {
      ConversationManager::AutoHoldMode autoHoldMode = myConversationManager.mDefaultAutoHoldMode;
      if (arg[0] == "n") autoHoldMode = ConversationManager::AutoHoldDisabled;
      else if (arg[0] == "y") autoHoldMode = ConversationManager::AutoHoldEnabled;
      else if (arg[0] == "b") autoHoldMode = ConversationManager::AutoHoldBroadcastOnly;
      myConversationManager.createConversation(autoHoldMode);
      return;
   }
   if (isEqualNoCase(command, "createsharedconv") || isEqualNoCase(command, "csc"))
   {
      unsigned long handle = arg[0].convertUnsignedLong();
      ConversationManager::AutoHoldMode autoHoldMode = myConversationManager.mDefaultAutoHoldMode;
      if (arg[1] == "n") autoHoldMode = ConversationManager::AutoHoldDisabled;
      else if (arg[0] == "y") autoHoldMode = ConversationManager::AutoHoldEnabled;
      else if (arg[1] == "b") autoHoldMode = ConversationManager::AutoHoldBroadcastOnly;
      mediaStackAdapter.createSharedMediaInterfaceConversation(handle, autoHoldMode);
      return;
   }
   if(isEqualNoCase(command, "destroyconv") || isEqualNoCase(command, "dc"))
   {
      unsigned long handle = arg[0].convertUnsignedLong();
      if(handle != 0)
      {
         myConversationManager.destroyConversation(handle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'destroyconv'|'dc'> <convHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "joinconv") || isEqualNoCase(command, "jc"))
   {
      unsigned long handleSrc = arg[0].convertUnsignedLong();
      unsigned long handleDest = arg[1].convertUnsignedLong();
      if(handleSrc != 0 && handleDest != 0)
      {
         myConversationManager.joinConversation(handleSrc, handleDest);
      }
      else
      {
         InfoLog( << "Invalid command format: <'joinconv'|'jc'> <sourceConvHandle> <destConvHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "createlocal") || isEqualNoCase(command, "clp"))
   {
      myConversationManager.createLocalParticipant();
      return;
   }
   if(isEqualNoCase(command, "createremote") || isEqualNoCase(command, "crp"))
   {
      unsigned long handle = arg[0].convertUnsignedLong();
      if(handle != 0 && !arg[1].empty())
      {
         ConversationManager::ParticipantForkSelectMode mode = ConversationManager::ForkSelectAutomaticEx;
         if(!arg[2].empty() && isEqualNoCase(arg[2], "manual"))
         {
            mode = ConversationManager::ForkSelectManual;
         }
         else if (!arg[2].empty() && isEqualNoCase(arg[2], "auto"))
         {
            mode = ConversationManager::ForkSelectAutomatic;
         }
         try
         {
            NameAddr dest(arg[1]);
            myConversationManager.createRemoteParticipant(handle, dest, mode);
         }
         catch(...)
         {
            NameAddr dest(uri);
            dest.uri().user() = arg[1];
            myConversationManager.createRemoteParticipant(handle, dest, mode);
         }
      }
      else
      {
         InfoLog( << "Invalid command format: <'createremote'|'crp'> <convHandle> <destURI> [<'manual|auto'>] (last arg is fork select mode, 'autoex' is default).");
      }
      return;
   }
   if (isEqualNoCase(command, "createremoteimpager") || isEqualNoCase(command, "crip"))
   {
      if (!arg[0].empty())
      {
         try
         {
            NameAddr dest(arg[0]);
            myConversationManager.createRemoteIMPagerParticipant(dest);
         }
         catch (...)
         {
            NameAddr dest(uri);
            dest.uri().user() = arg[0];
            myConversationManager.createRemoteIMPagerParticipant(dest);
         }
      }
      else
      {
         InfoLog(<< "Invalid command format: <'createremoteim'|'crip'> <destURI>");
      }
      return;
   }
   if (isEqualNoCase(command, "createremoteimsession") || isEqualNoCase(command, "cris"))
   {
      if (!arg[0].empty())
      {
         ConversationManager::ParticipantForkSelectMode mode = ConversationManager::ForkSelectAutomaticEx;
         if (!arg[1].empty() && isEqualNoCase(arg[1], "manual"))
         {
            mode = ConversationManager::ForkSelectManual;
         }
         else if (!arg[1].empty() && isEqualNoCase(arg[1], "auto"))
         {
            mode = ConversationManager::ForkSelectAutomatic;
         }
         try
         {
            NameAddr dest(arg[0]);
            myConversationManager.createRemoteIMSessionParticipant(dest, mode);
         }
         catch (...)
         {
            NameAddr dest(uri);
            dest.uri().user() = arg[0];
            myConversationManager.createRemoteIMSessionParticipant(dest, mode);
         }
      }
      else
      {
         InfoLog(<< "Invalid command format: <'createremoteimsession'|'cris'> <destURI> [<'manual|auto'>] (last arg is fork select mode, 'autoex' is default).");
      }
      return;
   }
   if(isEqualNoCase(command, "createmedia") || isEqualNoCase(command, "cmp"))
   {
      unsigned long handle = arg[0].convertUnsignedLong();
      unsigned long duration = arg[2].convertUnsignedLong();
      if(handle != 0 && !arg[1].empty())
      {
         try
         {
            Uri url(arg[1]);
            if(duration != 0)
            {
               url.param(p_duration) = duration;
            }
            myConversationManager.createMediaResourceParticipant(handle, url);
         }
         catch(resip::BaseException& e)
         {
            InfoLog( << "Invalid url format: <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]: " << e);
         }
         catch(...)
         {
            InfoLog( << "Invalid url format: <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]");
         }
      }
      else
      {
         //myConversationManager.createMediaResourceParticipant(1, Uri("http://www.sillyhumor.com/answer/helloo.wav"));
         InfoLog( << "Invalid command format: <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]");
      }
      return;
   }
   if(isEqualNoCase(command, "destroypart") || isEqualNoCase(command, "dp"))
   {
      unsigned long handle = arg[0].convertUnsignedLong();
      if(handle != 0)
      {
         myConversationManager.destroyParticipant(handle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'destroypart'|'dp'> <parthandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "addpart") || isEqualNoCase(command, "ap"))
   {
      unsigned long convHandle = arg[0].convertUnsignedLong();
      unsigned long partHandle = arg[1].convertUnsignedLong();
      if(convHandle != 0 && partHandle != 0)
      {
         myConversationManager.addParticipant(convHandle, partHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'addpart'|'ap'> <convHandle> <partHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "remotepart") || isEqualNoCase(command, "rp"))
   {
      unsigned long convHandle = arg[0].convertUnsignedLong();
      unsigned long partHandle = arg[1].convertUnsignedLong();
      if(convHandle != 0 && partHandle != 0)
      {
         myConversationManager.removeParticipant(convHandle, partHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'removepart'|'rp'> <convHandle> <partHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "movepart") || isEqualNoCase(command, "mp"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      unsigned long srcConvHandle = arg[1].convertUnsignedLong();
      unsigned long dstConvHandle = arg[2].convertUnsignedLong();
      if(partHandle != 0 && srcConvHandle != 0 && dstConvHandle != 0)
      {
         myConversationManager.moveParticipant(partHandle, srcConvHandle, dstConvHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'movepart'|'mp'> <partHandle> <srcConvHandle> <dstConvHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "partcontrib") || isEqualNoCase(command, "pc"))
   {
      unsigned long convHandle = arg[0].convertUnsignedLong();
      unsigned long partHandle = arg[1].convertUnsignedLong();
      if(partHandle != 0 && convHandle != 0)
      {
         myConversationManager.modifyParticipantContribution(convHandle, partHandle, arg[2].convertUnsignedLong(), arg[3].convertUnsignedLong());
      }
      else
      {
         InfoLog( << "Invalid command format: <'partcontrib'|'pc'> <convHandle> <partHandle> <inputGain> <outputGain> (gain in percentage)");
      }
      return;
   }
   if(isEqualNoCase(command, "bridgematrix") || isEqualNoCase(command, "bm"))
   {
      unsigned long convHandle = arg[0].convertUnsignedLong();
      myConversationManager.outputBridgeMatrix(convHandle);
      return;
   }   
   if(isEqualNoCase(command, "alert") || isEqualNoCase(command, "al"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      bool early = true;
      if(partHandle != 0)
      {
         if(!arg[1].empty() && isEqualNoCase(arg[1], "noearly"))
         {
            early = false;
         }
         myConversationManager.alertParticipant(partHandle, early);
      }
      else
      {
         InfoLog( << "Invalid command format: <'alert'|'al'> <partHandle> [<'noearly'>] (last arg is early flag, enabled by default)");
      }
      return;
   }
   if(isEqualNoCase(command, "answer") || isEqualNoCase(command, "an"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      if(partHandle != 0)
      {
         myConversationManager.answerParticipant(partHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'answer'|'an'> <partHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "reject") || isEqualNoCase(command, "rj"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      unsigned long status = arg[1].convertUnsignedLong();
      if(partHandle != 0)
      {
         if(status == 0) status = 486;
         myConversationManager.rejectParticipant(partHandle, status);
      }
      else
      {
         InfoLog( << "Invalid command format: <'reject'|'rj'> <partHandle> [<statusCode>] (default status code is 486)");
      }
      return;
   }
   if(isEqualNoCase(command, "redirect") || isEqualNoCase(command, "rd"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      if(partHandle != 0 && !arg[1].empty())
      {
         try
         {
            NameAddr dest(arg[1]);
            myConversationManager.redirectParticipant(partHandle, dest);
         }
         catch(...)
         {
            NameAddr dest(uri);
            dest.uri().user() = arg[1];
            myConversationManager.redirectParticipant(partHandle, dest);
         }
      }
      else
      {
         InfoLog( << "Invalid command format: <'redirect'|'rd'> <partHandle> <destURI>");
      }
      return;
   }
   if(isEqualNoCase(command, "redirectTo") || isEqualNoCase(command, "rt"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      unsigned long destPartHandle = arg[1].convertUnsignedLong();
      if(partHandle != 0 && destPartHandle != 0)
      {
         myConversationManager.redirectToParticipant(partHandle, destPartHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'redirectTo'|'rt'> <partHandle> <destPartHandle>");
      }
      return;
   }
   if (isEqualNoCase(command, "sendIM") || isEqualNoCase(command, "si"))
   {
      unsigned long partHandle = arg[0].convertUnsignedLong();
      Data plaintext = arg[1];
      if (partHandle != 0 && !plaintext.empty())
      {
         std::unique_ptr<PlainContents> plainContents(new PlainContents(plaintext));
         myConversationManager.sendIMToParticipant(partHandle, std::move(plainContents));
      }
      else
      {
         InfoLog(<< "Invalid command format: <'sendIM'|'si'> <partHandle> <plaintext>");
      }
      return;
   }
   if(isEqualNoCase(command, "volume") || isEqualNoCase(command, "sv"))
   {
      unsigned long volume = arg[0].convertUnsignedLong();
      mediaStackAdapter.setSpeakerVolume(volume);
      InfoLog( << "Speaker volume set to " << volume);
      return;
   }
   if(isEqualNoCase(command, "gain") || isEqualNoCase(command, "sg"))
   {
      unsigned long gain = arg[0].convertUnsignedLong();
      mediaStackAdapter.setMicrophoneGain(gain);
      InfoLog( << "Microphone gain set to " << gain);
      return;
   }
   if(isEqualNoCase(command, "mute") || isEqualNoCase(command, "mm"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      mediaStackAdapter.muteMicrophone(enable);
      InfoLog( << "Microphone mute " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "echocanel") || isEqualNoCase(command, "aec"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      mediaStackAdapter.enableEchoCancel(enable);
      InfoLog( << "Echo cancellation " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "autogain") || isEqualNoCase(command, "agc"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      mediaStackAdapter.enableAutoGainControl(enable);
      InfoLog( << "Automatic gain control " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "noisereduction") || isEqualNoCase(command, "nr"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      mediaStackAdapter.enableNoiseReduction(enable);
      return;
   }
   if(isEqualNoCase(command, "subscribe") || isEqualNoCase(command, "cs"))
   {
      unsigned int subTime = arg[2].convertUnsignedLong();
      if(!arg[0].empty() && !arg[1].empty() && subTime != 0 && !arg[3].empty() && !arg[4].empty())
      {
         try
         {
            NameAddr dest(arg[1]);
            Mime mime(arg[3], arg[4]);
            myUserAgent.createSubscription(arg[0], dest, subTime, mime);
         }
         catch(...)
         {
            NameAddr dest(uri);
            Mime mime(arg[3], arg[4]);
            dest.uri().user() = arg[1];
            myUserAgent.createSubscription(arg[0], dest, subTime, mime);
         }
      }
      else
      {
         InfoLog( << "Invalid command format: <'subscribe'|'cs'> <eventType> <targetUri> <subTime> <mimeType> <mimeSubType>");
      }
      return;
   }
   if(isEqualNoCase(command, "destsub") || isEqualNoCase(command, "ds"))
   {
      unsigned int subHandle = arg[0].convertUnsignedLong();

      if(subHandle > 0)
      {
         myUserAgent.destroySubscription(subHandle);
      }
      else
      {
         InfoLog( << "Invalid command format: <'destsub'|'ds'> <subHandle>");
      }
      return;
   }
   if(isEqualNoCase(command, "autoans") || isEqualNoCase(command, "aa"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      autoAnswerEnabled = enable;
      InfoLog( << "Autoanswer " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "setcodecs") || isEqualNoCase(command, "sc"))
   {
      Data codecId;
      std::vector<unsigned int> idList;
      ParseBuffer pb(arg[0]);
      pb.skipWhitespace();
      while(!pb.eof())
      {
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace, ",");  // white space or "," 
         pb.data(codecId, start);
         idList.push_back(codecId.convertUnsignedLong());
         if(!pb.eof())
         {
            pb.skipChar(',');
         }
      }
      unsigned int numCodecIds = (unsigned int)idList.size();
      if(numCodecIds > 0)
      {
         Data ipAddress(g_conversationProfile->sessionCaps().session().connection().getAddress());
         // Note:  Technically modifying the conversation profile at runtime like this is not
         //        thread safe.  But it should be fine for this test consoles purposes.
         mediaStackAdapter.buildSessionCapabilities(ipAddress, idList, g_conversationProfile->sessionCaps());
      }
      return;
   }
   if(isEqualNoCase(command, "securemedia") || isEqualNoCase(command, "sm"))
   {
      ConversationProfile::SecureMediaMode secureMediaMode = ConversationProfile::NoSecureMedia;
      bool secureMediaRequired = false;
      if(isEqualNoCase(arg[0], "Srtp"))
      {
         secureMediaMode = ConversationProfile::Srtp;
      }
      else if(isEqualNoCase(arg[0], "SrtpReq"))
      {
         secureMediaMode = ConversationProfile::Srtp;
         secureMediaRequired = true;
      }
#ifdef USE_SSL
      else if(isEqualNoCase(arg[0], "SrtpDtls"))
      {
         secureMediaMode = ConversationProfile::SrtpDtls;
      }
      else if(isEqualNoCase(arg[0], "SrtpDtlsReq"))
      {
         secureMediaMode = ConversationProfile::SrtpDtls;
         secureMediaRequired = true;
      }
#endif
      else
      {
         arg[0] = "None";  // for display output only
      }
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      g_conversationProfile->secureMediaMode() = secureMediaMode;
      g_conversationProfile->secureMediaRequired() = secureMediaRequired;      
      InfoLog( << "Secure media mode set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "natmode") || isEqualNoCase(command, "nm"))
   {
      ConversationProfile::NatTraversalMode natTraversalMode = ConversationProfile::NoNatTraversal;
      if(isEqualNoCase(arg[0], "Bind"))
      {
         natTraversalMode = ConversationProfile::StunBindDiscovery;
      }
      else if(isEqualNoCase(arg[0], "UdpAlloc"))
      {
         natTraversalMode = ConversationProfile::TurnUdpAllocation;
      }
      else if(isEqualNoCase(arg[0], "TcpAlloc"))
      {
         natTraversalMode = ConversationProfile::TurnTcpAllocation;
      }
#ifdef USE_SSL
      else if(isEqualNoCase(arg[0], "TlsAlloc"))
      {
         natTraversalMode = ConversationProfile::TurnTlsAllocation;
      }
#endif
      else
      {
         arg[0] = "None";  // for display output only
      }
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      g_conversationProfile->natTraversalMode() = natTraversalMode;
      InfoLog( << "NAT traversal mode set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "natserver") || isEqualNoCase(command, "ns"))
   {
      Data natTraversalServerHostname;
      unsigned short natTraversalServerPort = 8777;
      // Read server and port
      ParseBuffer pb(arg[0]);
      pb.skipWhitespace();
      const char *start = pb.position();
      pb.skipToOneOf(ParseBuffer::Whitespace, ":");  // white space or ":" 
      pb.data(natTraversalServerHostname, start);
      if(!pb.eof())
      {
         pb.skipChar(':');
         start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace);  // white space 
         Data port;
         pb.data(port, start);
         natTraversalServerPort = (unsigned short)port.convertUnsignedLong();
      }
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      g_conversationProfile->natTraversalServerHostname() = natTraversalServerHostname;
      g_conversationProfile->natTraversalServerPort() = natTraversalServerPort;
      InfoLog( << "NAT traversal STUN/TURN server set to: " << natTraversalServerHostname << ":" << natTraversalServerPort);
      return;
   }
   if(isEqualNoCase(command, "natuser") || isEqualNoCase(command, "nu"))
   {
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      g_conversationProfile->stunUsername() = arg[0];
      InfoLog( << "STUN/TURN user set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "natpwd") || isEqualNoCase(command, "np"))
   {
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      g_conversationProfile->stunPassword() = arg[0];
      InfoLog( << "STUN/TURN password set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "starttimer") || isEqualNoCase(command, "st"))
   {
      unsigned int timerId = arg[0].convertUnsignedLong();
      unsigned int durationMs = arg[1].convertUnsignedLong();
      unsigned int seqNumber = arg[2].convertUnsignedLong();

      if(durationMs > 0)
      {
         myUserAgent.startApplicationTimer(timerId, durationMs, seqNumber);
         InfoLog( << "Application Timer started for " << durationMs << "ms");
      }
      else
      {
         InfoLog( << "Invalid command format: <'starttimer'|'st'> <timerId> <durationMs> <seqNo>");
      }
      return;
   }
   if(isEqualNoCase(command, "info") || isEqualNoCase(command, "i"))
   {
      myConversationManager.displayInfo();
      return;
   }
   if(isEqualNoCase(command, "dns") || isEqualNoCase(command, "ld"))
   {
      InfoLog( << "DNS cache (at WARNING log level):");
      myUserAgent.logDnsCache();
      return;
   }
   if(isEqualNoCase(command, "cleardns") || isEqualNoCase(command, "cd"))
   {
      myUserAgent.clearDnsCache();
      InfoLog( << "DNS cache has been cleared.");
      return;
   }

#ifdef USE_SSL
   Data setSecureMediaMode("  setSecureMediaMode       <'securemedia'|'sm'> <'None'|'Srtp'|'SrtpReq'|'SrtpDtls'|'SrtpDtlsReq'>");
   Data setNATTraversalMode("  setNATTraversalMode      <'natmode'|'nm'> <'None'|'Bind'|'UdpAlloc'|'TcpAlloc'|'TlsAlloc'>" );
#else
   Data setSecureMediaMode("  setSecureMediaMode       <'securemedia'|'sm'> <'None'|'Srtp'|'SrtpReq'>");
   Data setNATTraversalMode("  setNATTraversalMode      <'natmode'|'nm'> <'None'|'Bind'|'UdpAlloc'|'TcpAlloc'>" );
#endif

   InfoLog( << "Possible commands are: " << endl
         << "  createConversation:      <'createconv'|'cc'> [<autoholdmode:'y'|'n'|'b'>]" << endl
         << "  createSharedMediaInterfaceConversation: <'createsharedconv'|'csc'> <convHandle> [<autoholdmode:'y'|'n'|'b'>]" << endl
         << "  destroyConversation:     <'destroyconv'|'dc'> <convHandle>" << endl
         << "  joinConversation:        <'joinconv'|'jc'> <sourceConvHandle> <destConvHandle>" << endl
         << endl 
         << "  createLocalParticipant:  <'createlocal'|'clp'>" << endl
         << "  createRemoteParticipant: <'createremote'|'crp'> <convHandle> <destURI> [<'manual|auto'>] (last arg is fork select mode, 'autoex' is default)" << endl 
         << "  createRemoteIMPagerParticipant:   <'createremoteimpager'|'crip'> <convHandle> <destURI>" << endl
         << "  createRemoteIMSessionParticipant: <'createremoteimsession'|'cris'> <destURI> [<'manual|auto'>] (last arg is fork select mode, 'autoex' is default)" << endl
         << "  createMediaResourceParticipant:   <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]" << endl
         << "  destroyParticipant:      <'destroypart'|'dp'> <parthandle>" << endl
         << endl 
         << "  addPartcipant:           <'addpart'|'ap'> <convHandle> <partHandle>" << endl
         << "  removePartcipant:        <'removepart'|'rp'> <convHandle> <partHandle>" << endl
         << "  moveParticipant:         <'movepart'|'mp'> <partHandle> <srcConvHandle> <dstConvHandle>" << endl
         << "  modifyParticipantContribution: <'partcontrib'|'pc'> <convHandle> <partHandle> <inputGain> <outputGain> (gain in percentage)" << endl
         << "  outputBridgeMatrix:      <'bridgematrix'|'bm'> [<convHandle>]" << endl
         << "  alertPartcipant:         <'alert'|'al'> <partHandle> [<'noearly'>] (last arg is early flag, enabled by default)" << endl
         << "  answerParticipant:       <'answer'|'an'> <partHandle>" << endl
         << "  rejectParticipant:       <'reject'|'rj'> <partHandle> [<statusCode>] (default status code is 486)" << endl
         << "  redirectPartcipant:      <'redirect'|'rd'> <partHandle> <destURI>" << endl
         << "  redirectToPartcipant:    <'redirectTo'|'rt'> <partHandle> <destPartHandle>" << endl
         << "  sendIMToParticipant:     <'sendIM'|'si'> <partHandle> <plaintext>" << endl
         << endl 
         << "  setSpeakerVolume:        <'volume'|'sv'> <volume>" << endl
         << "  setMicrophoneGain:       <'gain'|'sg'> <gain>" << endl
         << "  muteMicrophone:          <'mute'|'mm'> <'0'|'1'> (1 to enable/mute)" << endl
         << "  enableEchoCancel:        <'echocancel'|'aec'> <'0'|'1'> (1 to enable)" << endl
         << "  enableAutoGainControl:   <'autogain'|'agc'> <'0'|'1'> (1 to enable)" << endl
         << "  enableNoiseReduction:    <'noisereduction'|'nr'> <'0'|'1'> (1 to enable)" << endl
         << endl   
         << "  createSubscription:      <'subscribe'|'cs'> <eventType> <targetUri> <subTime> <mimeType> <mimeSubType>" << endl
         << "  destroySubscription:     <'destsub'|'ds'> <subHandle>" << endl
         << endl
         << "  setAutoAnswer            <'autoans'|'aa'> <'0'|'1'> (1 to enable (default))" << endl
         << "  setCodecs                <'setcodecs'|'sc'> <codecId>[,<codecId>]+ (comma separated list)" << endl
         << setSecureMediaMode << endl
         << setNATTraversalMode << endl
         << "  setNATTraversalServer    <'natserver'|'ns'> <server:port>" << endl
         << "  setNATUsername           <'natuser'|'nu'> <username>" << endl
         << "  setNATPassword           <'natpwd'|'np'> <password>" << endl
         << "  startApplicationTimer:   <'starttimer'|'st'> <timerId> <durationMs> <seqNo>" << endl
         << "  displayInfo:             <'info'|'i'>" << endl
         << "  logDnsCache:             <'dns'|'ld'>" << endl
         << "  clearDnsCache:           <'cleardns'|'cd'>" << endl
         << "  exitProgram:             <'exit'|'quit'|'q'|'x'>");
}

#define KBD_BUFFER_SIZE 256
void processKeyboard(char input, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent)
{
   static char buffer[KBD_BUFFER_SIZE];
   static int bufferpos = 0;

   if(input == 13 || input == 10)  // enter
   {
      Data db(buffer,bufferpos);
#ifdef WIN32
      cout << endl;
#endif
      processCommandLine(db, myConversationManager, myUserAgent);
      bufferpos = 0;
   }
   else if(input == 8 || input == 127) // backspace
   {
      if(bufferpos > 0)
      {
#ifdef WIN32
         cout << input << ' ' << input;
#else
         // note:  This is bit of a hack and may not be portable to all linux terminal types
         cout << "\b\b\b   \b\b\b";
         fflush(stdout);
#endif
         bufferpos--;
      }
   }
   else
   {
      if(bufferpos == KBD_BUFFER_SIZE) 
      {
         cout << endl;
         bufferpos = 0;
      }
      else
      {
#ifdef WIN32
         cout << input;
#endif
         buffer[bufferpos++] = (char)input;
      }
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

   initNetwork();

   // Defaults
   bool registrationDisabled = false;
   bool keepAlivesDisabled = false;
   Data password;
   Data dnsServers;
   Data address = DnsUtil::getLocalIpAddress();
   ConversationProfile::SecureMediaMode secureMediaMode = ConversationProfile::NoSecureMedia;
   bool secureMediaRequired = false;
   ConversationProfile::NatTraversalMode natTraversalMode = ConversationProfile::NoNatTraversal;
   Data natTraversalServerHostname;
   unsigned short natTraversalServerPort = 8777;
   Data stunUsername;
   Data stunPassword;
   bool localAudioEnabled = true;
   bool multipleMediaInterfaceModeEnabled = false;
   bool defaultAutoHoldModeToDisabled = false;
   unsigned short sipPort = 0;
   unsigned short tlsPort = 0;
   unsigned short mediaPortStart = 17384;
   Data tlsDomain = DnsUtil::getLocalHostName();
   NameAddr outboundProxy;
   Data logLevel("INFO");
   std::vector<unsigned int> codecIds = { SdpCodec::SDP_CODEC_PCMU /* 0 - pcmu */,
                               SdpCodec::SDP_CODEC_PCMA /* 8 - pcma */, 
                               SdpCodec::SDP_CODEC_G729A /* 18 - g729 */,
                               SdpCodec::SDP_CODEC_OPUS /* 147 - opus */,
                               SdpCodec::SDP_CODEC_SPEEX /* 96 - speex NB 8,000bps */,
                               SdpCodec::SDP_CODEC_SPEEX_15 /* 98 - speex NB 15,000bps */, 
                               SdpCodec::SDP_CODEC_SPEEX_24 /* 99 - speex NB 24,600bps */,
                               SdpCodec::SDP_CODEC_L16_44100_MONO /* PCM 16 bit/sample 44100 samples/sec. */, 
                               SdpCodec::SDP_CODEC_ILBC /* 108 - iLBC */,
                               SdpCodec::SDP_CODEC_ILBC_20MS /* 109 - Internet Low Bit Rate Codec, 20ms (RFC3951) */, 
                               SdpCodec::SDP_CODEC_SPEEX_5 /* 97 - speex NB 5,950bps */,
                               SdpCodec::SDP_CODEC_GSM /* 3 - GSM */,
                               //SdpCodec::SDP_CODEC_G722 /* 9 - G.722 */,
                               SdpCodec::SDP_CODEC_TONES /* 110 - telephone-event */};

   // Loop through command line arguments and process them
   for(int i = 1; i < argc; i++)
   {
      Data commandName(argv[i]);

      // Process all commandNames that don't take values
      if(isEqualNoCase(commandName, "-?") || 
         isEqualNoCase(commandName, "--?") ||
         isEqualNoCase(commandName, "--help") ||
         isEqualNoCase(commandName, "/?"))
      {
         cout << "Command line options are:" << endl;
         cout << " -aa - enable autoanswer" << endl;
         cout << " -a <IP Address> - bind SIP transports to this IP address" << endl;
         cout << " -u <SIP URI> - URI of this SIP user" << endl;
         cout << " -p <password> - SIP password of this this SIP user" << endl;
         cout << " -nr - no registration, set this to disable registration with SIP Proxy" << endl;
         cout << " -d <DNS servers> - comma seperated list of DNS servers, overrides OS detected list" << endl;
         cout << " -sp <port num> - local port number to use for SIP messaging (UDP/TCP)" << endl;
         cout << " -mp <port num> - local port number to start allocating from for RTP media" << endl;
#ifdef USE_SSL
         cout << " -tp <port num> - local port number to use for TLS SIP messaging" << endl;
         cout << " -td <domain name> - domain name to use for TLS server connections" << endl;
#endif
         cout << " -nk - no keepalives, set this to disable sending of keepalives" << endl;
         cout << " -op <SIP URI> - URI of a proxy server to use a SIP outbound proxy" << endl;
#ifdef USE_SSL
         cout << " -sm <Srtp|SrtpReq|SrtpDtls|SrtpDtlsReq> - sets the secure media mode" << endl;
         cout << " -nm <Bind|UdpAlloc|TcpAlloc|TlsAlloc> - sets the NAT traversal mode" << endl;
#else
         cout << " -sm <Srtp|SrtpReq> - sets the secure media mode" << endl;
         cout << " -nm <Bind|UdpAlloc|TcpAlloc> - sets the NAT traversal mode" << endl;
#endif
         cout << " -ns <server:port> - set the hostname and port of the NAT STUN/TURN server" << endl;
         cout << " -nu <username> - sets the STUN/TURN username to use for NAT server" << endl;
         cout << " -np <password> - sets the STUN/TURN password to use for NAT server" << endl;
         cout << " -nl - no local audio support - remove local sound hardware requirement" << endl;
         cout << " -mm - multiple media interface mode - enable for media server mode" << endl;
         cout << " -ad - default auto hold mode to disabled" << endl;
         cout << " -l <NONE|CRIT|ERR|WARNING|INFO|DEBUG|STACK> - logging level" << endl;
         cout << endl;
         cout << "Sample Command line:" << endl;
         cout << "testUA -a 192.168.1.100 -u sip:1000@myproxy.com -p 123 -aa" << endl;
         return 0;
      }
      else if(isEqualNoCase(commandName, "-nr"))
      {
         registrationDisabled = true;
      }
      else if(isEqualNoCase(commandName, "-aa"))
      {
         autoAnswerEnabled = true;
      }
      else if(isEqualNoCase(commandName, "-nk"))
      {
         keepAlivesDisabled = true;
      }
      else if(isEqualNoCase(commandName, "-nl"))
      {
         localAudioEnabled = false;
      }
      else if (isEqualNoCase(commandName, "-mm"))
      {
         multipleMediaInterfaceModeEnabled = true;
      }
      else if (isEqualNoCase(commandName, "-ad"))
      {
         defaultAutoHoldModeToDisabled = true;
      }
      else
      {
         // Process commands that have values
         Data commandValue(i+1 < argc ? argv[i+1] : Data::Empty);
         if(commandValue.empty() || commandValue.at(0) == '-')
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
         i++;  // increment argument

         if(isEqualNoCase(commandName, "-a"))
         {
            address = commandValue;
         }
         else if(isEqualNoCase(commandName, "-u"))
         {
            try
            {
               NameAddr tempuri(commandValue);
               uri = tempuri;
            }
            catch(resip::BaseException& e)
            {
               cerr << "Invalid uri format: " << e << endl;
               exit(-1);
            }
         }
         else if(isEqualNoCase(commandName, "-p"))
         {
            password = commandValue;
         }
         else if(isEqualNoCase(commandName, "-d"))
         {
            dnsServers = commandValue;
         }
         else if(isEqualNoCase(commandName, "-sm"))
         {
            if(isEqualNoCase(commandValue, "Srtp"))
            {
               secureMediaMode = ConversationProfile::Srtp;
            }
            else if(isEqualNoCase(commandValue, "SrtpReq"))
            {
               secureMediaMode = ConversationProfile::Srtp;
               secureMediaRequired = true;
            }
#ifdef USE_SSL
            else if(isEqualNoCase(commandValue, "SrtpDtls"))
            {
               secureMediaMode = ConversationProfile::SrtpDtls;
            }
            else if(isEqualNoCase(commandValue, "SrtpDtlsReq"))
            {
               secureMediaMode = ConversationProfile::SrtpDtls;
               secureMediaRequired = true;
            }
#endif
            else
            {
               cerr << "Invalid Secure Media Mode: " << commandValue << endl;
               exit(-1);
            }
         }
         else if(isEqualNoCase(commandName, "-nm"))
         {
            if(isEqualNoCase(commandValue, "Bind"))
            {
               natTraversalMode = ConversationProfile::StunBindDiscovery;
            }
            else if(isEqualNoCase(commandValue, "UdpAlloc"))
            {
               natTraversalMode = ConversationProfile::TurnUdpAllocation;
            }
            else if(isEqualNoCase(commandValue, "TcpAlloc"))
            {
               natTraversalMode = ConversationProfile::TurnTcpAllocation;
            }
#ifdef USE_SSL
            else if(isEqualNoCase(commandValue, "TlsAlloc"))
            {
               natTraversalMode = ConversationProfile::TurnTlsAllocation;
            }
#endif
            else
            {
               cerr << "Invalid NAT Traversal Mode: " << commandValue << endl;
               exit(-1);
            }
         }
         else if(isEqualNoCase(commandName, "-ns"))
         {
            // Read server and port
            Data natServerAndPort = commandValue;
            ParseBuffer pb(natServerAndPort);
            pb.skipWhitespace();
            const char *start = pb.position();
            pb.skipToOneOf(ParseBuffer::Whitespace, ":");  // white space or ":" 
            Data hostname;
            pb.data(hostname, start);
            natTraversalServerHostname = hostname;
            if(!pb.eof())
            {
               pb.skipChar(':');
               start = pb.position();
               pb.skipToOneOf(ParseBuffer::Whitespace);  // white space 
               Data port;
               pb.data(port, start);
               natTraversalServerPort = (unsigned short)port.convertUnsignedLong();
            }
         }
         else if(isEqualNoCase(commandName, "-nu"))
         {
            stunUsername = commandValue;
         }
         else if(isEqualNoCase(commandName, "-np"))
         {
            stunPassword = commandValue;
         }
         else if(isEqualNoCase(commandName, "-sp"))
         {
            sipPort = (unsigned short)commandValue.convertUnsignedLong();
         }
         else if(isEqualNoCase(commandName, "-mp"))
         {
            mediaPortStart = (unsigned short)commandValue.convertUnsignedLong();
         }
         else if(isEqualNoCase(commandName, "-tp"))
         {
            tlsPort = (unsigned short)commandValue.convertUnsignedLong();
         }
         else if(isEqualNoCase(commandName, "-td"))
         {
            tlsDomain = commandValue;
         }
         else if(isEqualNoCase(commandName, "-op"))
         {
            try
            {
               NameAddr tempuri(commandValue);
               outboundProxy = tempuri;
            }
            catch(resip::BaseException& e)
            {
               cerr << "Invalid outbound proxy uri format: " << e << endl;
               exit(-1);
            }
         }
         else if(isEqualNoCase(commandName, "-l"))
         {
            logLevel = commandValue;
         }
         else
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
      }
   }

   //enableConsoleOutput(TRUE);  // Allow sipX console output
   OsSysLog::initialize(0, "testUA");
   OsSysLog::setOutputFile(0, "sipXtapilog.txt") ;
   //OsSysLog::enableConsoleOutput(true);
   //OsSysLog::setLoggingPriority(PRI_DEBUG);
   Log::initialize("Cout", logLevel, "testUA");
   //UserAgent::setLogLevel(Log::Warning, UserAgent::SubsystemAll);
   //UserAgent::setLogLevel(Log::Info, UserAgent::SubsystemRecon);

   InfoLog( << "testUA settings:");
   InfoLog( << "  No Keepalives = " << (keepAlivesDisabled ? "true" : "false"));
   InfoLog( << "  Autoanswer = " << (autoAnswerEnabled ? "true" : "false"));
   InfoLog( << "  Do not register = " << (registrationDisabled ? "true" : "false"));
   InfoLog( << "  Local IP Address = " << address);
   InfoLog( << "  SIP URI = " << uri);
   InfoLog( << "  SIP Password = " << password);
   InfoLog( << "  Override DNS Servers = " << dnsServers);
   InfoLog( << "  Secure Media Mode = " << secureMediaMode);
   InfoLog( << "  NAT Traversal Mode = " << natTraversalMode);
   InfoLog( << "  NAT Server = " << natTraversalServerHostname << ":" << natTraversalServerPort);
   InfoLog( << "  STUN/TURN user = " << stunUsername);
   InfoLog( << "  STUN/TURN password = " << stunPassword);
   InfoLog( << "  SIP Port = " << sipPort);
   InfoLog( << "  Media Port Range Start = " << mediaPortStart);
#ifdef USE_SSL
   InfoLog( << "  TLS Port = " << tlsPort);
   InfoLog( << "  TLS Domain = " << tlsDomain);
#endif
   InfoLog( << "  Outbound Proxy = " << outboundProxy);
   InfoLog( << "  Local Audio Enabled = " << (localAudioEnabled ? "true" : "false"));
   InfoLog( << "  Multiple Media Interface Mode Enabled = " << (multipleMediaInterfaceModeEnabled ? "true" : "false"));
   InfoLog( << "  Default Auto Hold Mode = " << (defaultAutoHoldModeToDisabled ? "disabled" : "enabled"));
   InfoLog( << "  Log Level = " << logLevel);
   
   InfoLog( << "type help or '?' for list of accepted commands." << endl);

   //////////////////////////////////////////////////////////////////////////////
   // Setup UserAgentMasterProfile
   //////////////////////////////////////////////////////////////////////////////

   auto profile = std::make_shared<UserAgentMasterProfile>();

   // Add transports
   profile->addTransport(UDP, sipPort, V4, StunDisabled, address);
   profile->addTransport(TCP, sipPort, V4, StunDisabled, address);
#ifdef USE_SSL
   profile->addTransport(TLS, tlsPort, V4, StunDisabled, address, tlsDomain);
#endif

   // The following settings are used to avoid a kernel panic seen on an ARM embedded platform.
   // The kernel panic happens when either binding a udp socket to port 0 (OS selected),
   // or calling connect without first binding to a specific port.  There is code in the
   // resip transport selector that uses a utility UDP socket in order to determine
   // which interface should be used to route to a particular destination.  This code calls
   // connect with no bind.  By setting a fixed transport interface here that 
   // code will not be used.
   // The following line can be safely removed for other platforms
   //profile->setFixedTransportInterface(address);

   // Settings
   profile->setDefaultRegistrationTime(3600);
   profile->setDefaultFrom(uri);
   profile->setDigestCredential(uri.uri().host(), uri.uri().user(), password);

   // DNS Servers
   ParseBuffer pb(dnsServers);
   Data dnsServer;
   while(!dnsServers.empty() && !pb.eof())
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

   // Add ENUM Suffixes from setting string - use code similar to dns server
   //profile->addEnumSuffix(enumSuffix);

   if(!keepAlivesDisabled)
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
   profile->addSupportedMimeType(PRACK,  Mime("application", "sdp"));
   profile->addSupportedMimeType(PRACK,  Mime("multipart", "mixed"));
   profile->addSupportedMimeType(PRACK,  Mime("multipart", "signed"));
   profile->addSupportedMimeType(PRACK,  Mime("multipart", "alternative"));
   profile->addSupportedMimeType(UPDATE, Mime("application", "sdp"));
   profile->addSupportedMimeType(UPDATE, Mime("multipart", "mixed"));
   profile->addSupportedMimeType(UPDATE, Mime("multipart", "signed"));
   profile->addSupportedMimeType(UPDATE, Mime("multipart", "alternative"));
   profile->addSupportedMimeType(NOTIFY, Mime("message", "sipfrag"));
   profile->addSupportedMimeType(MESSAGE, Mime("text", "plain"));
   profile->addSupportedMimeType(MESSAGE, Mime("application", "im-iscomposing+xml"));

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
   profile->addSupportedMethod(PRACK);
   profile->addSupportedMethod(INFO);
   profile->addSupportedMethod(MESSAGE);

   profile->clearSupportedOptionTags();
   profile->addSupportedOptionTag(Token(Symbols::Replaces));      
   profile->addSupportedOptionTag(Token(Symbols::Timer)); 
   profile->addSupportedOptionTag(Token(Symbols::NoReferSub));
   profile->addSupportedOptionTag(Token(Symbols::AnswerMode));
   profile->addSupportedOptionTag(Token(Symbols::TargetDialog));
   //profile->addSupportedOptionTag(Token(Symbols::C100rel));  // Automatically added by calling setUacReliableProvisionalMode

   profile->setUacReliableProvisionalMode(MasterProfile::Supported);

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

   //profile->setOverrideHostAndPort(mContact);
   if(!outboundProxy.uri().host().empty())
   {
      profile->setOutboundProxy(outboundProxy.uri());
   }

   profile->setUserAgent("ConversationManager/TestUA");
   profile->rtpPortRangeMin() = mediaPortStart;
   profile->rtpPortRangeMax() = mediaPortStart + 101; // Allows 100 media streams

   //////////////////////////////////////////////////////////////////////////////
   // Setup ConversationProfile
   //////////////////////////////////////////////////////////////////////////////

   g_conversationProfile = std::make_shared<ConversationProfile>(profile);
   if(uri.uri().user() != "noreg" && !registrationDisabled)
   {
      g_conversationProfile->setDefaultRegistrationTime(3600);
   }
   else
   {
      g_conversationProfile->setDefaultRegistrationTime(0);
   }
   g_conversationProfile->setDefaultRegistrationRetryTime(120);  // 2 mins
   g_conversationProfile->setDefaultFrom(uri);
   g_conversationProfile->setDigestCredential(uri.uri().host(), uri.uri().user(), password);

   // Setup NatTraversal Settings
   g_conversationProfile->natTraversalMode() = natTraversalMode;
   g_conversationProfile->natTraversalServerHostname() = natTraversalServerHostname;
   g_conversationProfile->natTraversalServerPort() = natTraversalServerPort;
   g_conversationProfile->stunUsername() = stunUsername;
   g_conversationProfile->stunPassword() = stunPassword;

   // Secure Media Settings
   g_conversationProfile->secureMediaMode() = secureMediaMode;
   g_conversationProfile->secureMediaRequired() = secureMediaRequired;
   g_conversationProfile->secureMediaDefaultCryptoSuite() = ConversationProfile::SRTP_AES_CM_128_HMAC_SHA1_80;

   //////////////////////////////////////////////////////////////////////////////
   // Create ConverationManager and UserAgent
   //////////////////////////////////////////////////////////////////////////////
   {
      MyConversationManager myConversationManager(localAudioEnabled, multipleMediaInterfaceModeEnabled, defaultAutoHoldModeToDisabled);
      MyUserAgent ua(&myConversationManager, profile);
      SipXConversationManager& mediaStackAdapter = static_cast<SipXConversationManager&>(myConversationManager.getMediaStackAdapter());
      mediaStackAdapter.buildSessionCapabilities(address, codecIds, g_conversationProfile->sessionCaps());

      // Generate InstanceId appropriate for testing only.  Should be UUID that persists 
      // across machine re-starts and is unique to this application instance.  The one used 
      // here is only as unique as the hostname and the actual SIP listening port.  If someone runs two 
      // instances of this application on the same host and SIP port for the same Aor, then things will 
      // break.  See RFC5626 section 4.1
      Data hostnameAndPort = DnsUtil::getLocalHostName() + Data(profile->getTransports().front().mActualPort);
      Data instanceHash = hostnameAndPort.md5().uppercase();
      assert(instanceHash.size() == 32);
      Data instanceId(48, Data::Preallocate);
      instanceId += "<urn:uuid:";
      instanceId += instanceHash.substr(0, 8);
      instanceId += "-";
      instanceId += instanceHash.substr(8, 4);
      instanceId += "-";
      instanceId += instanceHash.substr(12, 4);
      instanceId += "-";
      instanceId += instanceHash.substr(16, 4);
      instanceId += "-";
      instanceId += instanceHash.substr(20, 12);
      instanceId += ">";
      g_conversationProfile->setInstanceId(instanceId);

      ua.addConversationProfile(g_conversationProfile);

      //////////////////////////////////////////////////////////////////////////////
      // Startup and run...
      //////////////////////////////////////////////////////////////////////////////

      ua.startup();
      myConversationManager.startup();

      //ua.createSubscription("message-summary", uri, 120, Mime("application", "simple-message-summary")); // thread safe

      int input;
      while(true)
      {
         ua.process(50);
         while(_kbhit() != 0)
         {
#ifdef WIN32
            input = _getch();
            processKeyboard(input, myConversationManager, ua);
#else
            input = fgetc(stdin);
            fflush(stdin);
            //cout << "input: " << input << endl;
            processKeyboard(input, myConversationManager, ua);
#endif
         }
         if(finished) break;
      }

      ua.shutdown();
   }
   InfoLog(LOG_PREFIX << "testUA is shutdown.");
   OsSysLog::shutdown();
   sleepSeconds(2);

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
} // end FML scope
#endif
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
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
