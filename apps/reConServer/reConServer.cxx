#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "resip/stack/InteropHelper.hxx"
#include "resip/recon/UserAgent.hxx"
#include "AppSubsystem.hxx"
#include <resip/recon/SipXHelper.hxx>

#include <os/OsSysLog.h>

#include "reConServerConfig.hxx"
#include "reConServer.hxx"
#include "MyMessageDecorator.hxx"
#include "MyConversationManager.hxx"
#include "B2BCallManager.hxx"
#include "CDRFile.hxx"
#include "RegistrationForwarder.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <rutil/WinLeakCheck.hxx>

#include <resip/stack/HEPSipMessageLoggingHandler.hxx>
#include <reflow/HEPRTCPEventLoggingHandler.hxx>

using namespace reconserver;
using namespace recon;
using namespace resip;
using namespace flowmanager;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

static bool finished = false;
NameAddr uri("sip:noreg@127.0.0.1");
bool autoAnswerEnabled = false;  // If enabled then reConServer will automatically answer incoming calls by adding to lowest numbered conversation
SharedPtr<ConversationProfile> conversationProfile;

int main(int argc, char** argv)
{
   ReConServerProcess proc;
   return proc.main(argc, argv);
}




ReConServerProcess::ReConServerProcess()
{
}

ReConServerProcess::~ReConServerProcess()
{
}

void ReConServerProcess::processCommandLine(Data& commandline, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent)
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

   // Process commands
   if(isEqualNoCase(command, "quit") || isEqualNoCase(command, "q") || isEqualNoCase(command, "exit"))
   {
      finished=true;
      return;
   }   
   if(isEqualNoCase(command, "createconv") || isEqualNoCase(command, "cc"))
   {
      myConversationManager.createConversation();
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
      ConversationManager::ParticipantForkSelectMode mode = ConversationManager::ForkSelectAutomatic;
      if(handle != 0 && !arg[1].empty())
      {
         if(!arg[2].empty() && isEqualNoCase(arg[2], "manual"))
         {
            mode = ConversationManager::ForkSelectManual;
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
         InfoLog( << "Invalid command format: <'createremote'|'crp'> <convHandle> <destURI> [<'manual'>] (last arg is fork select mode, 'auto' is default).");
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
      myConversationManager.outputBridgeMatrix();
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
   if(isEqualNoCase(command, "volume") || isEqualNoCase(command, "sv"))
   {
      unsigned long volume = arg[0].convertUnsignedLong();
      myConversationManager.setSpeakerVolume(volume);
      InfoLog( << "Speaker volume set to " << volume);
      return;
   }
   if(isEqualNoCase(command, "gain") || isEqualNoCase(command, "sg"))
   {
      unsigned long gain = arg[0].convertUnsignedLong();
      myConversationManager.setMicrophoneGain(gain);
      InfoLog( << "Microphone gain set to " << gain);
      return;
   }
   if(isEqualNoCase(command, "mute") || isEqualNoCase(command, "mm"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      myConversationManager.muteMicrophone(enable);
      InfoLog( << "Microphone mute " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "echocanel") || isEqualNoCase(command, "aec"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      myConversationManager.enableEchoCancel(enable);
      InfoLog( << "Echo cancellation " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "autogain") || isEqualNoCase(command, "agc"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      myConversationManager.enableAutoGainControl(enable);
      InfoLog( << "Automatic gain control " << (enable ? "enabled" : "disabled"));
      return;
   }
   if(isEqualNoCase(command, "noisereduction") || isEqualNoCase(command, "nr"))
   {
      bool enable = arg[0].convertUnsignedLong() != 0;
      myConversationManager.enableNoiseReduction(enable);
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
      std::list<unsigned int> idList;
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
      unsigned int numCodecIds = idList.size();
      if(numCodecIds > 0)
      {
         unsigned int* codecIdArray = new unsigned int[numCodecIds];
         unsigned int index = 0;
         std::list<unsigned int>::iterator it = idList.begin();
         for(;it != idList.end(); it++)
         {
            codecIdArray[index++] = (*it);
         }
         Data ipAddress(conversationProfile->sessionCaps().session().connection().getAddress());
         // Note:  Technically modifying the conversation profile at runtime like this is not
         //        thread safe.  But it should be fine for this test consoles purposes.
         myConversationManager.buildSessionCapabilities(ipAddress, numCodecIds, codecIdArray, conversationProfile->sessionCaps());
         delete [] codecIdArray;
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
      conversationProfile->secureMediaMode() = secureMediaMode;
      conversationProfile->secureMediaRequired() = secureMediaRequired;      
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
      conversationProfile->natTraversalMode() = natTraversalMode;
      InfoLog( << "NAT traversal mode set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "natserver") || isEqualNoCase(command, "ns"))
   {
      Data natTraversalServerHostname;
      unsigned short natTraversalServerPort = 3478;
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
         natTraversalServerPort = port.convertUnsignedLong();
      }
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      conversationProfile->natTraversalServerHostname() = natTraversalServerHostname;
      conversationProfile->natTraversalServerPort() = natTraversalServerPort;
      InfoLog( << "NAT traversal STUN/TURN server set to: " << natTraversalServerHostname << ":" << natTraversalServerPort);
      return;
   }
   if(isEqualNoCase(command, "natuser") || isEqualNoCase(command, "nu"))
   {
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      conversationProfile->stunUsername() = arg[0];
      InfoLog( << "STUN/TURN user set to: " << arg[0]);
      return;
   }
   if(isEqualNoCase(command, "natpwd") || isEqualNoCase(command, "np"))
   {
      // Note:  Technically modifying the conversation profile at runtime like this is not
      //        thread safe.  But it should be fine for this test consoles purposes.
      conversationProfile->stunPassword() = arg[0];
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
         << "  createConversation:      <'createconv'|'cc'>" << endl
         << "  destroyConversation:     <'destroyconv'|'dc'> <convHandle>" << endl
         << "  joinConversation:        <'joinconv'|'jc'> <sourceConvHandle> <destConvHandle>" << endl
         << endl 
         << "  createLocalParticipant:  <'createlocal'|'clp'>" << endl
         << "  createRemoteParticipant: <'createremote'|'crp'> <convHandle> <destURI> [<'manual'>] (last arg is fork select mode, 'auto' is default)" << endl 
         << "  createMediaResourceParticipant: <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]" << endl 
         << "  destroyParticipant:      <'destroypart'|'dp'> <parthandle>" << endl
         << endl 
         << "  addPartcipant:           <'addpart'|'ap'> <convHandle> <partHandle>" << endl
         << "  removePartcipant:        <'removepart'|'rp'> <convHandle> <partHandle>" << endl
         << "  moveParticipant:         <'movepart'|'mp'> <partHandle> <srcConvHandle> <dstConvHandle>" << endl
         << "  modifyParticipantContribution: <'partcontrib'|'pc'> <convHandle> <partHandle> <inputGain> <outputGain> (gain in percentage)" << endl
         << "  outputBridgeMatrix:      <'bridgematrix'|'bm'>" << endl
         << "  alertPartcipant:         <'alert'|'al'> <partHandle> [<'noearly'>] (last arg is early flag, enabled by default)" << endl
         << "  answerParticipant:       <'answer'|'an'> <partHandle>" << endl
         << "  rejectParticipant:       <'reject'|'rj'> <partHandle> [<statusCode>] (default status code is 486)" << endl
         << "  redirectPartcipant:      <'redirect'|'rd'> <partHandle> <destURI>" << endl
         << "  redirectToPartcipant:    <'redirectTo'|'rt'> <partHandle> <destPartHandle>" << endl
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
         << "  exitProgram:             <'exit'|'quit'|'q'>");
}

#define KBD_BUFFER_SIZE 256
void ReConServerProcess::processKeyboard(char input, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent)
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
ReConServerProcess::main (int argc, char** argv)
{
   installSignalHandler();

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
   {
#endif

   Data defaultConfigFilename("reConServer.config");
   ReConServerConfig reConServerConfig;
   try
   {
      reConServerConfig.parseConfig(argc, argv, defaultConfigFilename);
   }
   catch(std::exception& e)
   {
      ErrLog(<< "Exception parsing configuration: " << e.what());
      return -1;
   }

   Data pidFile = reConServerConfig.getConfigData("PidFile", "", true);
   bool daemonize = reConServerConfig.getConfigBool("Daemonize", false);
   mKeyboardInput = reConServerConfig.getConfigBool("KeyboardInput", !daemonize);
   if(daemonize && mKeyboardInput)
   {
      ErrLog(<< "Ignoring KeyboardInput=true setting as we are running as a daemon");
      mKeyboardInput = false;
   }
   setPidFile(pidFile);
   // Daemonize if necessary
   if(daemonize)
   {
      ReConServerProcess::daemonize();
   }

   autoAnswerEnabled = reConServerConfig.getConfigBool("EnableAutoAnswer", false);
   bool registrationDisabled = reConServerConfig.getConfigBool("DisableRegistration", false);
   bool keepAlivesDisabled = reConServerConfig.getConfigBool("DisableKeepAlives", false);
   Data password = reConServerConfig.getConfigData("Password", "", true);
   Data dnsServers = reConServerConfig.getConfigData("DNSServers", "", true);;
   Data address = reConServerConfig.getConfigData("IPAddress", DnsUtil::getLocalIpAddress(), true);
   ConversationProfile::SecureMediaMode secureMediaMode = reConServerConfig.getConfigSecureMediaMode("SecureMediaMode", ConversationProfile::NoSecureMedia);
   bool secureMediaRequired = reConServerConfig.isSecureMediaModeRequired();
   ConversationProfile::NatTraversalMode natTraversalMode = reConServerConfig.getConfigNatTraversalMode("NatTraversalMode", ConversationProfile::NoNatTraversal);
   bool forceCOMedia = reConServerConfig.getConfigBool("ForceCOMedia", true);
   Data natTraversalServerHostname = reConServerConfig.getConfigData("NatTraversalServerHostname", "", true);
   unsigned short natTraversalServerPort = reConServerConfig.getConfigUnsignedShort("NatTraversalServerPort", 3478);
   Data stunUsername = reConServerConfig.getConfigData("StunUsername", "", true);
   Data stunPassword = reConServerConfig.getConfigData("StunPassword", "", true);
   bool addViaRport = reConServerConfig.getConfigBool("AddViaRport", true);
   unsigned int maxReceiveFifoSize = reConServerConfig.getConfigInt("MaxReceiveFifoSize", 1000);
   unsigned short tcpPort = reConServerConfig.getConfigUnsignedShort("TCPPort", 5062);
   unsigned short udpPort = reConServerConfig.getConfigUnsignedShort("UDPPort", 5062);
   unsigned short tlsPort = reConServerConfig.getConfigUnsignedShort("TLSPort", 5063);
   unsigned short mediaPortStart = reConServerConfig.getConfigUnsignedShort("MediaPortStart", 17384);
   Data tlsDomain = reConServerConfig.getConfigData("TLSDomain", DnsUtil::getLocalHostName(), true);
   NameAddr outboundProxy = reConServerConfig.getConfigNameAddr("OutboundProxyUri", NameAddr(), true);
#ifdef PACKAGE_VERSION
   Data serverText = reConServerConfig.getConfigData("ServerText", "reConServer " PACKAGE_VERSION);
#else
   Data serverText = reConServerConfig.getConfigData("ServerText", "reConServer");
#endif
   uri = reConServerConfig.getConfigNameAddr("SIPUri", uri, true);
   Data loggingType = reConServerConfig.getConfigData("LoggingType", "cout", true);
   Data loggingLevel = reConServerConfig.getConfigData("LoggingLevel", "INFO", true);
   Data loggingFilename = reConServerConfig.getConfigData("LogFilename", "reConServer.log", true);
   unsigned int loggingFileMaxLineCount = reConServerConfig.getConfigUnsignedLong("LogFileMaxLines", 50000);
   Data cdrLogFilename = reConServerConfig.getConfigData("CDRLogFile", "", true);
   Data captureHost = reConServerConfig.getConfigData("CaptureHost", "");
   int capturePort = reConServerConfig.getConfigInt("CapturePort", 9060);
   int captureAgentID = reConServerConfig.getConfigInt("CaptureAgentID", 2002);
   bool localAudioEnabled = reConServerConfig.getConfigBool("EnableLocalAudio", !daemonize); // Defaults to false for daemon process
   Data runAsUser = reConServerConfig.getConfigData("RunAsUser", "", true);
   Data runAsGroup = reConServerConfig.getConfigData("RunAsGroup", "", true);
   ConversationManager::MediaInterfaceMode mediaInterfaceMode = reConServerConfig.getConfigBool("GlobalMediaInterface", false)
      ? ConversationManager::sipXGlobalMediaInterfaceMode : ConversationManager::sipXConversationMediaInterfaceMode;
   unsigned int defaultSampleRate = reConServerConfig.getConfigUnsignedLong("DefaultSampleRate", 8000);
   unsigned int maximumSampleRate = reConServerConfig.getConfigUnsignedLong("MaximumSampleRate", 8000);
   bool enableG722 = reConServerConfig.getConfigBool("EnableG722", false);
   bool enableOpus = reConServerConfig.getConfigBool("EnableOpus", false);
   ReConServerConfig::Application application = reConServerConfig.getConfigApplication("Application", ReConServerConfig::None);


   // build a list of codecs in priority order
   // Used by ConversationManager::buildSessionCapabilities(...) to create
   // our local SDP
   std::vector<unsigned int> _codecIds;
   if(enableOpus)
   {
      _codecIds.push_back(SdpCodec::SDP_CODEC_OPUS);        // Opus
   }
   if(enableG722)
   {
      _codecIds.push_back(SdpCodec::SDP_CODEC_G722);        // 9 - G.722
   }
   _codecIds.push_back(SdpCodec::SDP_CODEC_ILBC);           // 108 - iLBC
   _codecIds.push_back(SdpCodec::SDP_CODEC_ILBC_20MS);      // 109 - Internet Low Bit Rate Codec, 20ms (RFC3951)
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_24);       // 99 - speex NB 24,600bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_15);       // 98 - speex NB 15,000bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX);          // 96 - speex NB 8,000bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_5);        // 97 - speex NB 5,950bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_GSM);            // 3 - GSM
   //_codecIds.push_back(SdpCodec::SDP_CODEC_L16_44100_MONO); // PCM 16 bit/sample 44100 samples/sec.
   _codecIds.push_back(SdpCodec::SDP_CODEC_PCMU);           // 0 - pcmu
   _codecIds.push_back(SdpCodec::SDP_CODEC_PCMA);           // 8 - pcma
   _codecIds.push_back(SdpCodec::SDP_CODEC_G729);           // 18 - G.729
   _codecIds.push_back(SdpCodec::SDP_CODEC_TONES);          // 110 - telephone-event
   unsigned int *codecIds = &_codecIds[0];
   unsigned int numCodecIds = _codecIds.size();

   Log::initialize(loggingType, loggingLevel, argv[0], loggingFilename.c_str());
   Log::setMaxLineCount(loggingFileMaxLineCount);

   // Setup logging for the sipX media stack
   // It is bridged to the reSIProcate logger
   SipXHelper::setupLoggingBridge("reConServer");
   //UserAgent::setLogLevel(Log::Warning, UserAgent::SubsystemAll);
   //UserAgent::setLogLevel(Log::Info, UserAgent::SubsystemRecon);

   initNetwork();

   InfoLog( << "reConServer settings:");
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
   InfoLog( << "  Max RTP receive FIFO size = " << maxReceiveFifoSize);
   InfoLog( << "  STUN/TURN user = " << stunUsername);
   InfoLog( << "  STUN/TURN password = " << stunPassword);
   InfoLog( << "  TCP Port = " << tcpPort);
   InfoLog( << "  UDP Port = " << udpPort);
   InfoLog( << "  Media Port Range Start = " << mediaPortStart);
#ifdef USE_SSL
   InfoLog( << "  TLS Port = " << tlsPort);
   InfoLog( << "  TLS Domain = " << tlsDomain);
#endif
   InfoLog( << "  Outbound Proxy = " << outboundProxy);
   InfoLog( << "  Local Audio Enabled = " << (localAudioEnabled ? "true" : "false"));
   InfoLog( << "  Global Media Interface = " <<
      ((mediaInterfaceMode == ConversationManager::sipXGlobalMediaInterfaceMode) ? "true" : "false"));
   InfoLog( << "  Default sample rate = " << defaultSampleRate);
   InfoLog( << "  Maximum sample rate = " << maximumSampleRate);
   InfoLog( << "  Enable G.722 codec = " << (enableG722 ? "true" : "false"));
   InfoLog( << "  Enable Opus codec = " << (enableOpus ? "true" : "false"));
   InfoLog( << "  Log Type = " << loggingType);
   InfoLog( << "  Log Level = " << loggingLevel);
   InfoLog( << "  Log Filename = " << loggingFilename);
   InfoLog( << "  Daemonize = " << (daemonize ? "true" : "false"));
   InfoLog( << "  KeyboardInput = " << (mKeyboardInput ? "true" : "false"));
   InfoLog( << "  PidFile = " << pidFile);
   InfoLog( << "  Run as user = " << runAsUser);
   InfoLog( << "  Run as group = " << runAsGroup);
   InfoLog( << "type help or '?' for list of accepted commands." << endl);

   //////////////////////////////////////////////////////////////////////////////
   // Setup UserAgentMasterProfile
   //////////////////////////////////////////////////////////////////////////////

   SharedPtr<UserAgentMasterProfile> profile(new UserAgentMasterProfile);

   Data certPath;
   reConServerConfig.getConfigValue("CertificatePath", certPath);
   if(!certPath.empty())
   {
      profile->certPath() = certPath;
   }
   Data caDir;
   reConServerConfig.getConfigValue("CADirectory", caDir);
   if(!caDir.empty())
   {
      profile->rootCertDirectories().push_back(caDir);
   }
   Data caFile;
   reConServerConfig.getConfigValue("CAFile", caFile);
   if(!caFile.empty())
   {
      profile->rootCertBundles().push_back(caFile);
   }

   if(!captureHost.empty())
   {
      SharedPtr<HepAgent> agent(new HepAgent(captureHost, capturePort, captureAgentID));
      profile->setTransportSipMessageLoggingHandler(SharedPtr<HEPSipMessageLoggingHandler>(new HEPSipMessageLoggingHandler(agent)));
      profile->setRTCPEventLoggingHandler(SharedPtr<HEPRTCPEventLoggingHandler>(new HEPRTCPEventLoggingHandler(agent)));
   }

   // Add transports
   try
   {
      bool useEmailAsSIP = reConServerConfig.getConfigBool("TLSUseEmailAsSIP", false);

      // Check if advanced transport settings are provided
      ConfigParse::NestedConfigMap m = reConServerConfig.getConfigNested("Transport");
      DebugLog(<<"Found " << m.size() << " interface(s) defined in the advanced format");
      if(!m.empty())
      {
         // Sample config file format for advanced transport settings
         // Transport1Interface = 192.168.1.106:5061
         // Transport1Type = TLS
         // Transport1TlsDomain = sipdomain.com
         // Transport1TlsCertificate = /etc/ssl/crt/sipdomain.com.pem
         // Transport1TlsPrivateKey = /etc/ssl/private/sipdomain.com.pem
         // Transport1TlsPrivateKeyPassPhrase = <pwd>
         // Transport1TlsClientVerification = None
         // Transport1RcvBufLen = 2000

         const char *anchor;
         for(ConfigParse::NestedConfigMap::iterator it = m.begin();
            it != m.end();
            it++)
         {
            int idx = it->first;
            SipConfigParse tc(it->second);
            Data transportPrefix = "Transport" + idx;
            DebugLog(<< "checking values for transport: " << idx);
            Data interfaceSettings = tc.getConfigData("Interface", Data::Empty, true);

            // Parse out interface settings
            ParseBuffer pb(interfaceSettings);
            anchor = pb.position();
            pb.skipToEnd();
            pb.skipBackToChar(':');  // For IPv6 the last : should be the port
            pb.skipBackChar();
            if(!pb.eof())
            {
               Data ipAddr;
               Data portData;
               pb.data(ipAddr, anchor);
               pb.skipChar();
               anchor = pb.position();
               pb.skipToEnd();
               pb.data(portData, anchor);
               if(!DnsUtil::isIpAddress(ipAddr))
               {
                  CritLog(<< "Malformed IP-address found in " << transportPrefix << "Interface setting: " << ipAddr);
               }
               int port = portData.convertInt();
               if(port == 0)
               {
                  CritLog(<< "Invalid port found in " << transportPrefix << " setting: " << port);
               }
               TransportType tt = Tuple::toTransport(tc.getConfigData("Type", "UDP"));
               if(tt == UNKNOWN_TRANSPORT)
               {
                  CritLog(<< "Unknown transport type found in " << transportPrefix << "Type setting: " << tc.getConfigData("Type", "UDP"));
               }
               Data tlsDomain = tc.getConfigData("TlsDomain", Data::Empty);
               Data tlsCertificate = tc.getConfigData("TlsCertificate", Data::Empty);
               Data tlsPrivateKey = tc.getConfigData("TlsPrivateKey", Data::Empty);
               Data tlsPrivateKeyPassPhrase = tc.getConfigData("TlsPrivateKeyPassPhrase", Data::Empty);
               SecurityTypes::TlsClientVerificationMode cvm = tc.getConfigClientVerificationMode("TlsClientVerification", SecurityTypes::None);
               SecurityTypes::SSLType sslType = SecurityTypes::NoSSL;
#ifdef USE_SSL
               sslType = tc.getConfigSSLType("TlsConnectionMethod", SecurityTypes::SSLv23);
#endif

               int rcvBufLen = tc.getConfigInt("RcvBufLen", 0);

               profile->addTransport(tt,
                                 port,
                                 DnsUtil::isIpV6Address(ipAddr) ? V6 : V4,
                                 StunEnabled,
                                 ipAddr,       // interface to bind to
                                 tlsDomain,
                                 tlsPrivateKeyPassPhrase,  // private key passphrase
                                 sslType, // sslType
                                 0,            // transport flags
                                 tlsCertificate, tlsPrivateKey,
                                 cvm,          // tls client verification mode
                                 useEmailAsSIP, rcvBufLen);

            }
            else
            {
               CritLog(<< "Port not specified in " << transportPrefix << " setting: expected format is <IPAddress>:<Port>");
               return false;
            }
         }
      }
      else
      {
         DebugLog(<<"Using legacy transport configuration");
         if(udpPort)
         {
            profile->addTransport(UDP, udpPort, V4, StunEnabled, address, Data::Empty, Data::Empty, SecurityTypes::SSLv23, 0, Data::Empty, Data::Empty, SecurityTypes::None, useEmailAsSIP);
         }
         if(tcpPort)
         {
            profile->addTransport(TCP, tcpPort, V4, StunEnabled, address, Data::Empty, Data::Empty, SecurityTypes::SSLv23, 0, Data::Empty, Data::Empty, SecurityTypes::None, useEmailAsSIP);
         }
#ifdef USE_SSL
         if(tlsPort)
         {
            profile->addTransport(TLS, tlsPort, V4, StunEnabled, address, tlsDomain, Data::Empty, SecurityTypes::SSLv23, 0, Data::Empty, Data::Empty, SecurityTypes::None, useEmailAsSIP);
         }
#endif
      }
   }
   catch (BaseException& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      return false;
   }

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
   profile->addSupportedMimeType(INFO, Mime("application", "dtmf-relay"));

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
   //profile->addSupportedMethod(MESSAGE);

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

   profile->setUserAgent(serverText);
   profile->rtpPortRangeMin() = mediaPortStart;
   profile->rtpPortRangeMax() = mediaPortStart + 101; // Allows 100 media streams

   if(natTraversalMode == ConversationProfile::NoNatTraversal)
   {
      StackLog(<<"NAT traversal features not enabled, "
         "adding message decorator for SDP connection address");
      SharedPtr<MessageDecorator> md(new MyMessageDecorator());
      profile->setOutboundDecorator(md);
   }

   //////////////////////////////////////////////////////////////////////////////
   // Setup ConversationProfile
   //////////////////////////////////////////////////////////////////////////////

   conversationProfile = SharedPtr<ConversationProfile>(new ConversationProfile(profile));
   if(uri.uri().user() != "noreg" && !registrationDisabled)
   {
      conversationProfile->setDefaultRegistrationTime(3600);
   }
   else
   {
      conversationProfile->setDefaultRegistrationTime(0);
   }
   conversationProfile->setDefaultRegistrationRetryTime(120);  // 2 mins
   conversationProfile->setDefaultFrom(uri);
   conversationProfile->setDigestCredential(uri.uri().host(), uri.uri().user(), password);

#if 0  // Now auto-built 

   // Create Session Capabilities and assign to coversation Profile
   // Note:  port, sessionId and version will be replaced in actual offer/answer   int port = 16384;
   // Build s=, o=, t=, and c= lines
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, SdpContents::IP4, address);   // o=   
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, address);  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", port, 1, "RTP/AVP");
   if(enableOpus)
   {
      // Note: the other constructors (e.g. g722 above) are
      // invoked incorrectly, payload format should be the second
      // argument or the rate is used as the payload format and
      // the desired rate is not used at all
      SdpContents::Session::Codec opuscodec("OPUS", 96, 48000);
      opuscodec.encodingParameters() = Data("2");
      medium.addCodec(opuscodec);
   }
   if(enableG722)
   {
      SdpContents::Session::Codec g722codec("G722", 8000);
      g722codec.payloadType() = 9;  /* RFC3551 */ ;
      medium.addCodec(g722codec);
   }
   SdpContents::Session::Codec speexCodec("SPEEX", 8000);
   speexCodec.payloadType() = 110;
   speexCodec.parameters() = Data("mode=3");
   medium.addCodec(speexCodec);
   SdpContents::Session::Codec gsmCodec("GSM", 8000);
   gsmCodec.payloadType() = 3;  /* RFC3551 */ ;
   medium.addCodec(gsmCodec);
   SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;  /* RFC3551 */ ;
   medium.addCodec(g711ucodec);
   SdpContents::Session::Codec g711acodec("PCMA", 8000);
   g711acodec.payloadType() = 8;  /* RFC3551 */ ;
   medium.addCodec(g711acodec);
   SdpContents::Session::Codec g729codec("G729", 8000);
   g729codec.payloadType() = 18;  /* RFC3551 */ ;
   medium.addCodec(g729codec);
   medium.addAttribute("ptime", Data(20));  // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
   medium.addAttribute("sendrecv");

   SdpContents::Session::Codec toneCodec("telephone-event", 8000);
   toneCodec.payloadType() = 102;  
   toneCodec.parameters() = Data("0-15");
   medium.addCodec(toneCodec);
   session.addMedium(medium);

   conversationProfile->sessionCaps().session() = session;
#endif

   // Setup NatTraversal Settings
   conversationProfile->natTraversalMode() = natTraversalMode;
   conversationProfile->forceCOMedia() = forceCOMedia;
   conversationProfile->natTraversalServerHostname() = natTraversalServerHostname;
   conversationProfile->natTraversalServerPort() = natTraversalServerPort;
   conversationProfile->stunUsername() = stunUsername;
   conversationProfile->stunPassword() = stunPassword;
   InteropHelper::setRportEnabled(addViaRport);
   conversationProfile->setRportEnabled(addViaRport);

   // Secure Media Settings
   conversationProfile->secureMediaMode() = secureMediaMode;
   conversationProfile->secureMediaRequired() = secureMediaRequired;
   conversationProfile->secureMediaDefaultCryptoSuite() = ConversationProfile::SRTP_AES_CM_128_HMAC_SHA1_80;

   Flow::maxReceiveFifoSize = maxReceiveFifoSize;

   //////////////////////////////////////////////////////////////////////////////
   // Create ConverationManager and UserAgent
   //////////////////////////////////////////////////////////////////////////////
   {
      B2BCallManager *b2BCallManager = 0;
      switch(application)
      {
         case ReConServerConfig::None:
            mConversationManager.reset(new MyConversationManager(localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maximumSampleRate, autoAnswerEnabled));
            break;
         case ReConServerConfig::B2BUA:
            {
               if(!cdrLogFilename.empty())
               {
                  mCDRFile.reset(new CDRFile(cdrLogFilename));
               }
               b2BCallManager = new B2BCallManager(mediaInterfaceMode, defaultSampleRate, maximumSampleRate, reConServerConfig, mCDRFile);
               mConversationManager.reset(b2BCallManager);
            }
            break;
         default:
            assert(0);
      }
      mUserAgent.reset(new MyUserAgent(reConServerConfig, mConversationManager.get(), profile));
      mConversationManager->buildSessionCapabilities(address, numCodecIds, codecIds, conversationProfile->sessionCaps());
      mUserAgent->addConversationProfile(conversationProfile);

      if(application == ReConServerConfig::B2BUA)
      {
         b2BCallManager->init(*mUserAgent.get());

         Data internalMediaAddress;
         reConServerConfig.getConfigValue("B2BUAInternalMediaAddress", internalMediaAddress);
         if(!internalMediaAddress.empty())
         {
            SharedPtr<ConversationProfile> internalProfile(new ConversationProfile(conversationProfile));
            Data b2BUANextHop = reConServerConfig.getConfigData("B2BUANextHop", "", true);
            if(b2BUANextHop.empty())
            {
               CritLog(<<"Please specify B2BUANextHop");
               throw ConfigParse::Exception("Please specify B2BUANextHop", __FILE__, __LINE__);
            }
            NameAddrs route;
            route.push_front(NameAddr(b2BUANextHop));
            internalProfile->setServiceRoute(route);
            internalProfile->secureMediaMode() = reConServerConfig.getConfigSecureMediaMode("B2BUAInternalSecureMediaMode", secureMediaMode);
            internalProfile->setDefaultFrom(uri);
            internalProfile->setDigestCredential(uri.uri().host(), uri.uri().user(), password);
            mConversationManager->buildSessionCapabilities(internalMediaAddress, numCodecIds, codecIds, internalProfile->sessionCaps());
            mUserAgent->addConversationProfile(internalProfile, false);
         }
         else
         {
            WarningLog(<<"B2BUAInternalMediaAddress not specified, using same media address for internal and external zones");
         }
      }

      //////////////////////////////////////////////////////////////////////////////
      // Startup and run...
      //////////////////////////////////////////////////////////////////////////////

      mUserAgent->startup();
      mConversationManager->startup();

      //mUserAgent->createSubscription("message-summary", uri, 120, Mime("application", "simple-message-summary")); // thread safe

      // Drop privileges (can do this now that sockets are bound)
      if(!runAsUser.empty())
      {
         InfoLog( << "Trying to drop privileges, configured uid = " << runAsUser << " gid = " << runAsGroup);
         dropPrivileges(runAsUser, runAsGroup);
      }

      mainLoop();

      mUserAgent->shutdown();
   }
   InfoLog(<< "reConServer is shutdown.");
   OsSysLog::shutdown();
   ::sleepSeconds(2);

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
} // end FML scope
#endif

   return 0;
}

void
ReConServerProcess::doWait()
{
   mUserAgent->process(50);
}

void
ReConServerProcess::onLoop()
{
   if(mKeyboardInput)
   {
      int input;
      while(_kbhit() != 0)
      {
#ifdef WIN32
         input = _getch();
         processKeyboard(input, *mConversationManager, *mUserAgent);
#else
         input = fgetc(stdin);
         fflush(stdin);
         //cout << "input: " << input << endl;
         processKeyboard(input, *mConversationManager, *mUserAgent);
#endif
      }
   }
}

void
ReConServerProcess::onReload()
{
   StackLog(<<"ReConServerProcess::onReload invoked");
   if(mCDRFile)
   {
      StackLog(<<"ReConServerProcess::onReload: request CDR rotation");
      mCDRFile->rotateLog();
   }
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
