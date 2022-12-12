#include "../UserAgent.hxx"
#include "AppSubsystem.hxx"
#include "ParkManager.hxx"
#include "ParkOrbit.hxx"
#include "Server.hxx"

#include <resip/stack/ExtensionParameter.hxx>
#include <resip/stack/SipMessage.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER

#define PARKLOG_PREFIX << "ParkManager[" << mParkUri << "] "

static const resip::ExtensionParameter p_orbit("orbit");
static const resip::ExtensionParameter p_automaton("automaton");
static const resip::ExtensionParameter p_byeless("+sip.byeless");
static const resip::ExtensionParameter p_rendering("+sip.rendering");

namespace mohparkserver 
{

ParkManager::ParkManager(Server& server) :
   mServer(server),
   mConversationProfileHandle(0),
   mOrbitRangeStart(0),
   mNumOrbits(0),
   mMaxParkTime(0)
{
}

ParkManager::~ParkManager()
{
}

void
ParkManager::startup(ConfigParser::ParkSettings& settings)
{
   // Initialize park settings
   initializeParkSettings(settings.mMaxParkTime, settings.mMOHFilenameUrl);

   // Setup ConversationProfile - If Park setting specific outbound proxy is not specified, then use global 
   // outbound proxy setting.
   initializeConversationProfile(settings.mUri, settings.mPassword, settings.mRegistrationTime,
      !settings.mOutboundProxy.uri().host().empty() ? settings.mOutboundProxy : mServer.mConfig.mOutboundProxy);

   // Create Orbit Profiles
   initializeOrbitConversationProfiles(settings.mOrbitRangeStart, settings.mNumOrbits, settings.mUri, settings.mOrbitPassword, settings.mOrbitRegistrationTime, 
      !settings.mOutboundProxy.uri().host().empty() ? settings.mOutboundProxy : mServer.mConfig.mOutboundProxy); 

   InfoLog(PARKLOG_PREFIX << "startup: MOHFilenameUrl=" << settings.mMOHFilenameUrl);
}

void 
ParkManager::initializeConversationProfile(const NameAddr& uri, const Data& password, unsigned long registrationTime, const resip::NameAddr& outboundProxy)
{
   Lock lock(mMutex);

   if(mConversationProfileHandle)
   {
      mServer.mMyUserAgent->destroyConversationProfile(mConversationProfileHandle);
      mConversationProfileHandle = 0;
   }

   auto parkConversationProfile = std::make_shared<ConversationProfile>(mServer.mUserAgentMasterProfile);
   parkConversationProfile->setDefaultRegistrationTime(registrationTime);  
   parkConversationProfile->setDefaultRegistrationRetryTime(120);  // 2 mins
   parkConversationProfile->setDefaultFrom(uri);
   parkConversationProfile->setDigestCredential(uri.uri().host(), uri.uri().user(), password);  
   if(!outboundProxy.uri().host().empty())
   {
      parkConversationProfile->setOutboundProxy(outboundProxy.uri());
   }
   parkConversationProfile->challengeOODReferRequests() = false;
   parkConversationProfile->setExtraHeadersInReferNotifySipFragEnabled(true);  // Enable dialog identifying headers in SipFrag bodies of Refer Notifies
   NameAddr capabilities;
   capabilities.param(p_automaton);
   capabilities.param(p_byeless);
   capabilities.param(p_rendering) = "\"no\"";
   parkConversationProfile->setUserAgentCapabilities(capabilities);  // Same as above
   parkConversationProfile->natTraversalMode() = ConversationProfile::NoNatTraversal;
   parkConversationProfile->secureMediaMode() = ConversationProfile::NoSecureMedia;
   mServer.buildSessionCapabilities(parkConversationProfile->sessionCaps());   
   mConversationProfileHandle = mServer.mMyUserAgent->addConversationProfile(std::move(parkConversationProfile));
   mParkUri = uri;
}

void 
ParkManager::initializeOrbitConversationProfiles(unsigned long orbitStart, 
                                                 unsigned long numOrbits, 
                                                 const NameAddr& uri, 
                                                 const Data& password, 
                                                 unsigned long registrationTime,
                                                 const resip::NameAddr& outboundProxy)
{
   Lock lock(mMutex);

   // Remove current Profiles (if set)
   OrbitProfileMap::iterator itOrbit = mOrbitProfiles.begin();
   for(;itOrbit!=mOrbitProfiles.end();itOrbit++)
   {
       mServer.mMyUserAgent->destroyConversationProfile(itOrbit->second);
   }
   mOrbitProfiles.clear();

   // Store orbit range values
   mOrbitRangeStart = orbitStart;
   mNumOrbits = numOrbits;

   // Clear free list and rebuild it
   mFreeOrbitList.clear();

   for(unsigned long orbit = mOrbitRangeStart; orbit < mOrbitRangeStart + mNumOrbits; orbit++)
   {
      auto orbitConversationProfile = std::make_shared<ConversationProfile>(mServer.mUserAgentMasterProfile);
      Data orbitData((uint64_t)orbit);
      orbitConversationProfile->setDefaultRegistrationTime(registrationTime);  
      orbitConversationProfile->setDefaultRegistrationRetryTime(120);  // 2 mins
      orbitConversationProfile->setDefaultFrom(uri);
      orbitConversationProfile->getDefaultFrom().uri().user() = orbitData;
      orbitConversationProfile->setDigestCredential(uri.uri().host(), orbitData, password);  
      if(!outboundProxy.uri().host().empty())
      {
         orbitConversationProfile->setOutboundProxy(outboundProxy.uri());
      }
      orbitConversationProfile->challengeOODReferRequests() = false;
      orbitConversationProfile->setExtraHeadersInReferNotifySipFragEnabled(true);  // Enable dialog identifying headers in SipFrag bodies of Refer Notifies
      NameAddr capabilities;
      capabilities.param(p_automaton);
      capabilities.param(p_byeless);
      capabilities.param(p_rendering) = "\"no\"";
      orbitConversationProfile->setUserAgentCapabilities(capabilities);  // Same as above
      orbitConversationProfile->natTraversalMode() = ConversationProfile::NoNatTraversal;
      orbitConversationProfile->secureMediaMode() = ConversationProfile::NoSecureMedia;
      mServer.buildSessionCapabilities(orbitConversationProfile->sessionCaps());      
      mOrbitProfiles[orbit] = mServer.mMyUserAgent->addConversationProfile(std::move(orbitConversationProfile));

      // If orbit is free - add to free list
      if(mOrbits.find(orbit) == mOrbits.end())      
      {
         mFreeOrbitList.push_back(orbit);
      }
   }
}

void 
ParkManager::initializeParkSettings(unsigned long maxParkTime, const resip::Uri& musicFilename)
{
   Lock lock(mMutex);
   mMaxParkTime = maxParkTime;
   mMusicFilename = musicFilename;
}

void 
ParkManager::shutdown(bool shuttingDownServer)
{
   Lock lock(mMutex);
   OrbitMap::iterator it = mOrbits.begin();
   for(;it!=mOrbits.end();it++)
   {
       // Delete all Orbit objects
       delete it->second;
   }
   mOrbits.clear();
   mOrbitsByParticipant.clear();

   // If shutting down server, then we shouldn't remove the conversation profiles here
   // shutting down the ConversationManager will take care of this.  We need to be sure
   // we don't remove all conversation profiles when we are still processing SipMessages,
   // since recon requires at least one to be present for inbound processing.
   if(!shuttingDownServer)
   {
      // Destroy main Park profile
      if(mConversationProfileHandle)
      {
          mServer.mMyUserAgent->destroyConversationProfile(mConversationProfileHandle);
          mConversationProfileHandle = 0;
      }

      // Destroy Orbit Profiles
      OrbitProfileMap::iterator itOrbit = mOrbitProfiles.begin();
      for(;itOrbit!=mOrbitProfiles.end();itOrbit++)
      {
          mServer.mMyUserAgent->destroyConversationProfile(itOrbit->second);
      }
      mOrbitProfiles.clear();
   }
   InfoLog(PARKLOG_PREFIX << "shutdown");
}

bool 
ParkManager::isMyProfile(recon::ConversationProfile& profile)
{
   Lock lock(mMutex);
   if(profile.getHandle() == mConversationProfileHandle)
   {
      return true;
   }
   else
   {
      // check orbit profiles
      OrbitProfileMap::iterator it = mOrbitProfiles.begin();
      for(;it!=mOrbitProfiles.end();it++)
      {
         if(profile.getHandle() == it->second)
         {
            return true;
         }
      }
   }
   return false;
}

ParkOrbit* 
ParkManager::getOrbit(unsigned long orbit)
{
   resip_assert((orbit >= mOrbitRangeStart) && 
          (orbit < (mOrbitRangeStart + mNumOrbits)));

   // Check if Orbit is created or not yet
   OrbitMap::iterator it = mOrbits.find(orbit);
   ParkOrbit* parkOrbit = 0;
   if(it == mOrbits.end())
   {
      // Create Orbit
      parkOrbit = new ParkOrbit(mServer, orbit, mMaxParkTime, mMusicFilename);
      mOrbits[orbit] = parkOrbit;

      // Remove from free list
      std::deque<unsigned long>::iterator itFree = mFreeOrbitList.begin();
      for(;itFree!=mFreeOrbitList.end();itFree++)
      {
         if(orbit == *itFree)
         {
            mFreeOrbitList.erase(itFree);
            break;
         }
      }
   }
   else
   {
      // Use existing
      parkOrbit = it->second;
   }
   return parkOrbit;
}

ParkOrbit* 
ParkManager::getOrbitByParticipant(recon::ParticipantHandle participantHandle)
{
   OrbitsByParticipantMap::iterator it = mOrbitsByParticipant.find(participantHandle);
   if(it != mOrbitsByParticipant.end())
   {
      return it->second;
   }
   return 0;
}

bool 
ParkManager::addParticipantToOrbit(ParkOrbit* orbit, recon::ParticipantHandle participantHandle, const resip::Uri& parkedUri, const resip::Uri& parkerUri)
{
   if(orbit->addParticipant(participantHandle, parkedUri, parkerUri))
   {
      mOrbitsByParticipant[participantHandle] = orbit;  // add participant to orbit index
      return true;
   }
   return false;
}

void 
ParkManager::parkParticipant(ParticipantHandle participantHandle, const SipMessage& msg)
{
   Lock lock(mMutex);

   unsigned long orbit = 0;
   resip_assert(msg.method() == REFER);

   // Check if Orbit parameter has been specified on the To header
   if(msg.header(h_To).uri().exists(p_orbit))
   {
      orbit = msg.header(h_To).uri().param(p_orbit).convertUnsignedLong();
   }

   if((orbit >= mOrbitRangeStart) && 
      (orbit < (mOrbitRangeStart + mNumOrbits)))
   {
      // Park call at specified orbit
      ParkOrbit* parkOrbit = getOrbit(orbit);
      resip_assert(parkOrbit);
      addParticipantToOrbit(parkOrbit, participantHandle, msg.header(h_ReferTo).uri().getAorAsUri(), msg.header(h_From).uri());
      InfoLog(PARKLOG_PREFIX << "parkParticipant: parked at orbit=" << orbit);
   }
   else
   {
      // If no orbit was specified, or specified number is bad - select a free orbit, and redirect request to use newly allocated orbit
      if(mFreeOrbitList.size() > 0)
      {
         unsigned long freeorbit = mFreeOrbitList.front();
         // Move free item to end of list, to reduce chance it will be double allocated
         mFreeOrbitList.pop_front();  
         mFreeOrbitList.push_back(freeorbit);
         InfoLog(PARKLOG_PREFIX << "parkParticipant: no valid orbit specified (orbit=" << orbit << ") redirecting to free orbit=" << freeorbit);
         NameAddr destination(mParkUri);
         destination.uri().param(p_orbit) = Data((uint64_t)freeorbit);
         mServer.redirectParticipant(participantHandle, destination);
      }
      else
      {
         // No free orbits
         WarningLog(PARKLOG_PREFIX << "parkParticipant: no free orbits available, rejecing with 486 busy.");
         mServer.rejectParticipant(participantHandle, 486 /* Busy */);
      }
   }
}

void 
ParkManager::incomingParticipant(ParticipantHandle participantHandle, const SipMessage& msg)
{
   Lock lock(mMutex);

   // Get orbit number, either from To Uri parameter, or To user 
   unsigned long orbit = 0;
   if(msg.header(h_To).uri().exists(p_orbit))
   {
      orbit = msg.header(h_To).uri().param(p_orbit).convertUnsignedLong();
   }
   else
   {
      orbit = msg.header(h_To).uri().user().convertUnsignedLong();
   }
   if((orbit >= mOrbitRangeStart) && 
      (orbit < (mOrbitRangeStart + mNumOrbits)))
   {
      // Check if this is a direct call, or a transferred call.  We will allow transferred calls to be parked, as an alternative parking method
      if(msg.exists(h_ReferredBy))
      {
         // If a Referred-By header is present then this was a transferred call - park it
         // Park call at specified orbit
         ParkOrbit* parkOrbit = getOrbit(orbit);
         resip_assert(parkOrbit);
         addParticipantToOrbit(parkOrbit, participantHandle, msg.header(h_From).uri(), msg.header(h_ReferredBy).uri());
         InfoLog(PARKLOG_PREFIX << "incomingParticipant: parked at orbit=" << orbit);
      }
      else  // Direct call - retrieval attempt
      {
         // Orbit is valid - see if we have a call parked there
         OrbitMap::iterator it = mOrbits.find(orbit);
         ParticipantHandle participantToRetrieve = 0;
         if(it!=mOrbits.end() && (participantToRetrieve = it->second->getNextQueuedParticipant()) != 0)
         {
            // Answer incoming call and then immediately redirect to parked call
            mServer.addParticipant(it->second->getConversationHandle(), participantHandle);
            mServer.modifyParticipantContribution(it->second->getConversationHandle(), participantHandle, 0, 0 /* Mute participant */);
            mServer.answerParticipant(participantHandle);
            mServer.redirectToParticipant(participantHandle, participantToRetrieve);

            InfoLog(PARKLOG_PREFIX << "incomingParticipant: retrieving participant " << participantToRetrieve << " from orbit " << orbit);
         }
         else
         {
            WarningLog(PARKLOG_PREFIX << "incomingParticipant: orbit " << orbit << " has no call to retrieve, rejecting with 404.");
            mServer.rejectParticipant(participantHandle, 404 /* Not Found */);
         }
      }
   }
   else
   {
      WarningLog(PARKLOG_PREFIX << "incomingParticipant: valid orbit not found in To header (" << msg.header(h_To).uri() << "), rejecting with 404.");
      mServer.rejectParticipant(participantHandle, 404 /* Not Found */);
   }
}

bool 
ParkManager::removeParticipant(ParticipantHandle participantHandle)
{
   Lock lock(mMutex);
   ParkOrbit* orbit = getOrbitByParticipant(participantHandle);
   if(orbit)
   {
      mOrbitsByParticipant.erase(participantHandle);  // Remove participant from orbit index
      if(orbit->removeParticipant(participantHandle))
      {
         InfoLog(PARKLOG_PREFIX << "removeParticipant: from orbit " << orbit->getOrbit() << " with " << orbit->getNumParticipants() << " remaining participants");
         if(orbit->getNumParticipants() == 0)
         {
            // Last participant just left orbit - destroy it
            unsigned long orbitNum = orbit->getOrbit();
            if((orbitNum >= mOrbitRangeStart) && 
               (orbitNum < (mOrbitRangeStart + mNumOrbits)))
            {
                // Only add back to free list, if in currently configured range
               mFreeOrbitList.push_back(orbitNum); 
            }
            delete orbit;
            mOrbits.erase(orbitNum);
         }
         return true;
      }
   }
   return false;
}

void 
ParkManager::getActiveCallsInfo(CallInfoList& callInfos)
{
   Lock lock(mMutex);

   OrbitMap::iterator it = mOrbits.begin();
   for(; it != mOrbits.end(); it++)
   {
       ParkOrbit::ParticipantQueue::iterator it2 = it->second->mParticipants.begin();
       for(; it2 != it->second->mParticipants.end(); it2++)
       {
          callInfos.push_back(ActiveCallInfo((*it2)->mParkedUri, 
                                             (*it2)->mParkerUri, 
                                             Data("Parked at " + Data((uint64_t)(it->first))), 
                                             (*it2)->mParticipantHandle, 
                                             it->second->mConversationHandle));
       }
   }
}

bool 
ParkManager::onMaxParkTimeout(recon::ParticipantHandle participantHandle)
{
   Lock lock(mMutex);

   // Try to see if participant is still around
   ParkOrbit* orbit = getOrbitByParticipant(participantHandle);
   if(orbit)
   {
      orbit->onMaxParkTimeout(participantHandle);
      return true;
   }
   return false;
}


}

/* ====================================================================

 Copyright (c) 2011-2016, SIP Spectrum, Inc.
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

