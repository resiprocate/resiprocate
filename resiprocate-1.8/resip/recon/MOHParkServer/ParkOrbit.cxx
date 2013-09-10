#include "../UserAgent.hxx"
#include "AppSubsystem.hxx"
#include "ParkOrbit.hxx"
#include "Server.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER
#define ALLOCATIONTIME 32  // The number of seconds to condsider a participant allocated, when a retreival is attempted
                           // If the call fails to retrieve, it will still be available to be retrieved by
                           // another attempt after this time expires.
namespace mohparkserver 
{

ParkOrbit::ParkOrbit(Server& server, unsigned long orbit, unsigned long maxParkTime, const resip::Uri& musicFilename) :
   mServer(server),
   mOrbit(orbit),
   mMaxParkTime(maxParkTime)
{
   // Create an initial conversation and start music
   mConversationHandle = mServer.createConversation(true /* broadcast only*/);

   // Play Music
   mServer.createMediaResourceParticipant(mConversationHandle, musicFilename);

   InfoLog(<< "ParkOrbit::ParkOrbit created orbit " << mOrbit);
}

ParkOrbit::~ParkOrbit()
{
   mServer.destroyConversation(mConversationHandle);
   InfoLog(<< "ParkOrbit::~ParkOrbit destroyed orbit " << mOrbit);
}

bool 
ParkOrbit::addParticipant(recon::ParticipantHandle participantHandle, const Uri& parkedUri, const Uri& parkerUri)
{
   if(mParticipants.size() < DEFAULT_BRIDGE_MAX_IN_OUTPUTS-3)
   {
      mServer.addParticipant(mConversationHandle, participantHandle);
      mServer.modifyParticipantContribution(mConversationHandle, participantHandle, 100, 0 /* Mute participant */);
      mServer.answerParticipant(participantHandle);

      if(mMaxParkTime != 0)
      {
         mServer.getMyUserAgent()->startApplicationTimer(MAXPARKTIMEOUT, mMaxParkTime*1000, participantHandle);
      }

      mParticipants.push_back(new ParticipantOrbitInfo(participantHandle, parkedUri, parkerUri));
      InfoLog(<< "ParkOrbit::addParticipant added participant=" << participantHandle << " to orbit " << mOrbit << " (size=" << mParticipants.size() << "), parkedUri=" << parkedUri << ", parkerUri=" << parkerUri);
      return true;
   }
   else
   {
      mServer.rejectParticipant(participantHandle, 486);
      WarningLog(<< "ParkOrbit::addParticipant cannot add participant=" << participantHandle << " to full orbit " << mOrbit << " (size=" << mParticipants.size() << ")");
      return false;
   }
}

bool 
ParkOrbit::removeParticipant(recon::ParticipantHandle participantHandle)
{
   ParticipantQueue::iterator it = mParticipants.begin();
   for(;it!=mParticipants.end();it++)
   {
      if((*it)->mParticipantHandle == participantHandle)
      {
         delete *it;
         mParticipants.erase(it); 
         InfoLog(<< "ParkOrbit::removeParticipant removed participant=" << participantHandle << " from orbit " << mOrbit << " (size=" << mParticipants.size() << ")");
         return true;
      }
   }
   return false;
}

ParticipantHandle 
ParkOrbit::getNextQueuedParticipant()
{
   ParticipantHandle participantHandle = 0;
   if(!mParticipants.empty())
   {
      UInt64 now = resip::Timer::getTimeSecs();
      if(now - mParticipants.front()->mAllocationTime > ALLOCATIONTIME)
      {
         participantHandle = mParticipants.front()->mParticipantHandle;
         mParticipants.front()->mAllocationTime = now;
         // Move to back of queue
         mParticipants.push_back(mParticipants.front());
         mParticipants.pop_front();
         InfoLog(<< "ParkOrbit::getNextQueuedParticipant removed participant=" << participantHandle << " from orbit " << mOrbit << " (size=" << mParticipants.size() << ")");
      }
   }
   return participantHandle;
}

bool 
ParkOrbit::onMaxParkTimeout(recon::ParticipantHandle participantHandle)
{
   ParticipantQueue::iterator it = mParticipants.begin();
   for(;it!=mParticipants.end();it++)
   {
      if((*it)->mParticipantHandle == participantHandle)
      {
         InfoLog(<< "ParkOrbit::onMaxParkTimeout sending parked call back to " << (*it)->mParkerUri << ", participant=" << participantHandle << " from orbit " << mOrbit << " (size=" << mParticipants.size() << ")");
         mServer.redirectParticipant(participantHandle, NameAddr((*it)->mParkerUri));
         (*it)->mAllocationTime = resip::Timer::getTimeSecs();  // Ensure call can't be retrieved between now and destruction
         mServer.destroyParticipant(participantHandle);  // Fully blind transfer - don't wait for notifies
         return true;
      }
   }
   return false;
}

}

/* ====================================================================

 Copyright (c) 2011, SIP Spectrum, Inc.
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

