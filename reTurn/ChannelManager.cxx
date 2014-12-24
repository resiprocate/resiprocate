#include "ChannelManager.hxx"
#include <rutil/Random.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN
#define TURN_CHANNEL_BINDING_LIFETIME_SECONDS 600   // 10 minuntes
//#define TURN_CHANNEL_BINDING_LIFETIME_SECONDS 60   // TESTING only

using namespace std;

namespace reTurn {

ChannelManager::ChannelManager()
{
   // make starting channel number random
   int randInt = resip::Random::getRandom();
   mNextChannelNumber = MIN_CHANNEL_NUM + (unsigned short)(randInt % (MAX_CHANNEL_NUM-MIN_CHANNEL_NUM+1));
}

ChannelManager::~ChannelManager()
{
   // Cleanup RemotePeer Memory
   TupleRemotePeerMap::iterator it;   
   for(it = mTupleRemotePeerMap.begin(); it != mTupleRemotePeerMap.end(); it++)
   {
      delete it->second;
   }
}

unsigned short 
ChannelManager::getNextChannelNumber() 
{ 
   if(mNextChannelNumber == MAX_CHANNEL_NUM)
   {
      mNextChannelNumber = MIN_CHANNEL_NUM;
   }
   else
   {
      mNextChannelNumber++; 
   }
   return mNextChannelNumber;
}

RemotePeer*
ChannelManager::createChannelBinding(const StunTuple& peerTuple)
{
   return createChannelBinding(peerTuple, getNextChannelNumber());
}

RemotePeer*
ChannelManager::createChannelBinding(const StunTuple& peerTuple, unsigned short channel)
{
   resip_assert(findRemotePeerByPeerAddress(peerTuple) == 0);

   // Create New RemotePeer
   RemotePeer* remotePeer = new RemotePeer(peerTuple, channel, TURN_CHANNEL_BINDING_LIFETIME_SECONDS);

   // Add RemoteAddress to the appropriate maps
   mTupleRemotePeerMap[peerTuple] = remotePeer;
   mChannelRemotePeerMap[channel] = remotePeer;
   return remotePeer;
}

RemotePeer* 
ChannelManager::findRemotePeerByChannel(unsigned short channelNumber)
{
   ChannelRemotePeerMap::iterator it = mChannelRemotePeerMap.find(channelNumber);
   if(it != mChannelRemotePeerMap.end())
   {
      if(!it->second->isExpired())
      {
         return it->second;
      }
      else
      {
         // cleanup expired channel binding
         mTupleRemotePeerMap.erase(it->second->getPeerTuple());
		 delete it->second;
         mChannelRemotePeerMap.erase(it);
      }
   }
   return 0;
}

RemotePeer* 
ChannelManager::findRemotePeerByPeerAddress(const StunTuple& peerAddress)
{
   // Find RemotePeer
   TupleRemotePeerMap::iterator it = mTupleRemotePeerMap.find(peerAddress);
   if(it != mTupleRemotePeerMap.end())
   {
      if(!it->second->isExpired())
      {
         return it->second;
      }
      else
      {
         // cleanup expired channel binding
         mChannelRemotePeerMap.erase(it->second->getChannel());
		 delete it->second;
         mTupleRemotePeerMap.erase(it);
      }
   }
   return 0;
}

} // namespace


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

