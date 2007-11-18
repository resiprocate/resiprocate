#include "ChannelManager.hxx"
#include <rutil/Random.hxx>

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
   {
      TupleRemotePeerMap::iterator it;   
      for(it = mTupleRemotePeerMap.begin(); it != mTupleRemotePeerMap.end(); it++)
      {
         delete it->second;
      }
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

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

