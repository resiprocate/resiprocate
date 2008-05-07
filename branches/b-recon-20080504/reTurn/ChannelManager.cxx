#include "ChannelManager.hxx"
#include <rutil/Random.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

#define MAX_CHANNEL_NUM 65534

ChannelManager::ChannelManager()
{
   // make starting channel number random
   int randInt = resip::Random::getRandom();
   mNextChannelNumber = (unsigned short)(randInt % (MAX_CHANNEL_NUM+1));
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
      mNextChannelNumber = 1;
   }
   else
   {
      mNextChannelNumber++; 
   }
   return mNextChannelNumber;
}

RemotePeer*
ChannelManager::createRemotePeer(const StunTuple& peerTuple, unsigned short clientToServerChannel, unsigned short serverToClientChannel)
{
   assert(findRemotePeerByPeerAddress(peerTuple) == 0);

   // Create New RemotePeer
   RemotePeer* remotePeer = new RemotePeer(peerTuple, clientToServerChannel, serverToClientChannel);

   // Add RemoteAddress to the appropriate maps
   mTupleRemotePeerMap[peerTuple] = remotePeer;
   if(clientToServerChannel != 0)
   {
      addRemotePeerClientToServerChannelLookup(remotePeer);
   }
   if(serverToClientChannel != 0)
   {
      addRemotePeerServerToClientChannelLookup(remotePeer);
   }
   return remotePeer;
}

void 
ChannelManager::addRemotePeerServerToClientChannelLookup(RemotePeer* remotePeer)
{
   assert(remotePeer->getServerToClientChannel() != 0);
   assert(findRemotePeerByServerToClientChannel(remotePeer->getServerToClientChannel()) == 0);
   mServerToClientChannelRemotePeerMap[remotePeer->getServerToClientChannel()] = remotePeer;
}

void 
ChannelManager::addRemotePeerClientToServerChannelLookup(RemotePeer* remotePeer)
{
   assert(remotePeer->getClientToServerChannel() != 0);
   assert(findRemotePeerByClientToServerChannel(remotePeer->getClientToServerChannel()) == 0);
   mClientToServerChannelRemotePeerMap[remotePeer->getClientToServerChannel()] = remotePeer;
}

RemotePeer* 
ChannelManager::findRemotePeerByServerToClientChannel(unsigned short channelNumber)
{
   ChannelRemotePeerMap::iterator it = mServerToClientChannelRemotePeerMap.find(channelNumber);
   if(it != mServerToClientChannelRemotePeerMap.end())
   {
      return it->second;
   }
   return 0;
}

RemotePeer* 
ChannelManager::findRemotePeerByClientToServerChannel(unsigned short channelNumber)
{
   ChannelRemotePeerMap::iterator it = mClientToServerChannelRemotePeerMap.find(channelNumber);
   if(it != mClientToServerChannelRemotePeerMap.end())
   {
      return it->second;
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
      return it->second;
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

