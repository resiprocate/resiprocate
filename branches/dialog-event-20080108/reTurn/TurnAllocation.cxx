#include <boost/bind.hpp>

#include "TurnAllocation.hxx"
#include "TurnManager.hxx"
#include "TurnPermission.hxx"
#include "AsyncSocketBase.hxx"
#include "UdpRelayServer.hxx"
#include "RemotePeer.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

#define TURN_PERMISSION_INACTIVITY_SECONDS 60   // .slg. move to configuration

namespace reTurn {

TurnAllocation::TurnAllocation(TurnManager& turnManager,
                               AsyncSocketBase* localTurnSocket,
                               const StunTuple& clientLocalTuple, 
                               const StunTuple& clientRemoteTuple,
                               const StunAuth& clientAuth, 
                               const StunTuple& requestedTuple, 
                               unsigned int lifetime,
                               UInt32 requestedPortProps, 
                               UInt32 requestedTransport, 
                               asio::ip::address* requestedIpAddress) :
   mKey(clientLocalTuple, clientRemoteTuple),
   mClientAuth(clientAuth),
   mRequestedTuple(requestedTuple),
   mRequestedPortProps(requestedPortProps),
   mRequestedTransport(requestedTransport),
   mTurnManager(turnManager),
   mAllocationTimer(turnManager.getIOService()),
   mLocalTurnSocket(localTurnSocket)
{
   if(requestedIpAddress)
   {
      mRequestedIpAddress = *requestedIpAddress;
   }

   InfoLog(<< "TurnAllocation created: clientLocal=" << clientLocalTuple << " clientRemote=" << 
           clientRemoteTuple << " requested=" << requestedTuple << " lifetime=" << lifetime);

   refresh(lifetime);

   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      mUdpRelayServer.reset(new UdpRelayServer(turnManager.getIOService(), *this));
      mUdpRelayServer->start();
   }
   else
   {
      ErrLog(<< "Only UDP relay's are currently implemented!");
      assert(false);
   }

   // Register for Turn Transport onDestroyed notification
   mLocalTurnSocket->registerAsyncSocketBaseHandler(this);
}

TurnAllocation::~TurnAllocation()
{
   InfoLog(<< "TurnAllocation destroyed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple);

   // Delete Relay Servers
   if(mUdpRelayServer) mUdpRelayServer->stop();

   // Deallocate Port
   mTurnManager.deallocatePort(mRequestedTuple.getTransportType(), mRequestedTuple.getPort());

   // Cleanup Permission Memory
   TurnPermissionMap::iterator it;   
   for(it = mTurnPermissionMap.begin(); it != mTurnPermissionMap.end(); it++)
   {
      delete it->second;
   }
   
   // Unregister for TurnTransport notifications
   mLocalTurnSocket->registerAsyncSocketBaseHandler(0);
}

void  
TurnAllocation::refresh(unsigned int lifetime)  // update expiration time
{
   InfoLog(<< "TurnAllocation refreshed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " lifetime=" << lifetime);

   mExpires = time(0) + lifetime;

   // start timer
   mAllocationTimer.expires_from_now(boost::posix_time::seconds(lifetime));
   mAllocationTimer.async_wait(boost::bind(&TurnManager::allocationExpired, &mTurnManager, asio::placeholders::error, mKey));
}

bool 
TurnAllocation::existsPermission(const asio::ip::address& address)
{
   TurnPermissionMap::iterator it = mTurnPermissionMap.find(address);
   if(it != mTurnPermissionMap.end())
   {
      if(it->second->isExpired()) // check if expired
      {
         InfoLog(<< "TurnAllocation has expired permission: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " exipred address=" << it->first);
         delete it->second;
         mTurnPermissionMap.erase(it);
         return false;
      }
      return true;
   }
   return false;
}

void 
TurnAllocation::refreshPermission(const asio::ip::address& address)
{
   TurnPermissionMap::iterator it = mTurnPermissionMap.find(address);
   TurnPermission* turnPermission = 0;
   if(it != mTurnPermissionMap.end())
   {
      turnPermission = it->second;
   }
   if(!turnPermission) // create if doesn't exist
   {
      mTurnPermissionMap[address] = new TurnPermission(address, TURN_PERMISSION_INACTIVITY_SECONDS);  
   }
   else
   {
      turnPermission->refresh();
   }
}

void 
TurnAllocation::onSocketDestroyed()
{
   mTurnManager.removeTurnAllocation(mKey);   // will delete this
}

void 
TurnAllocation::sendDataToPeer(unsigned short channelNumber, boost::shared_ptr<DataBuffer> data, bool framed)
{
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByClientToServerChannel(channelNumber);
   if(remotePeer)
   {
      // channel found - send Data
      sendDataToPeer(remotePeer->getPeerTuple(), data, framed);
   }
   else
   {
      WarningLog(<< "sendDataToPeer bad channel number - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
         mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << channelNumber);
   }
}

void 
TurnAllocation::sendDataToPeer(unsigned short channelNumber, const StunTuple& peerAddress, boost::shared_ptr<DataBuffer> data, bool framed)
{
   // Find RemotePeer
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(peerAddress);
   if(remotePeer)
   {
      // Found Remote Peer - ensure channel number matches
      if(channelNumber != remotePeer->getClientToServerChannel())
      {
         WarningLog(<< "sendDataToPeer channel number mismatch - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" <<  mRequestedTuple << " oldChannelNumber=" << remotePeer->getClientToServerChannel() <<
            " newChannelNumber=" << channelNumber);
         // drop message
         return;
      }
   }
   else
   {
      RemotePeer* remotePeer = mChannelManager.createRemotePeer(peerAddress, channelNumber, 0);
      assert(remotePeer);
      remotePeer->setClientToServerChannelConfirmed();
   }

   if(mKey.getClientLocalTuple().getTransportType() == StunTuple::UDP)
   {
      // If UDP, then send TurnChannelConfirmationInd
      StunMessage channelConfirmationInd;
      channelConfirmationInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnChannelConfirmationMethod);
      channelConfirmationInd.mHasTurnPeerAddress = true;
      StunMessage::setStunAtrAddressFromTuple(channelConfirmationInd.mTurnPeerAddress, peerAddress);
      channelConfirmationInd.mHasTurnChannelNumber = true;
      channelConfirmationInd.mTurnChannelNumber = channelNumber;

      // send channelConfirmationInd to local client
      unsigned int bufferSize = 8 /* Stun Header */ + 36 /* Remote Address (v6) */ + 8 /* TurnChannelNumber */ + 4 /* Turn Frame size */;
      boost::shared_ptr<DataBuffer> buffer = AsyncSocketBase::allocateBuffer(bufferSize);
      unsigned int size = channelConfirmationInd.stunEncodeFramedMessage((char*)buffer->data(), bufferSize);
      buffer->truncate(size);  // Set size to proper size
      mLocalTurnSocket->doSend(mKey.getClientRemoteTuple(), buffer);
   }      

   sendDataToPeer(peerAddress, data, framed);
}

void 
TurnAllocation::sendDataToPeer(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer> data, bool framed)
{
   DebugLog(<< "TurnAllocation sendDataToPeer: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " peerAddress=" << peerAddress);

   // add permission if it does not exist
   refreshPermission(peerAddress.getAddress());

   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      assert(mUdpRelayServer);
      mUdpRelayServer->doSend(peerAddress, data, framed ? 4 /* bufferStartPos is 4 so that framing is skipped */ : 0);
   }
   else
   {
      if(data->size() <=4)
      {
         WarningLog(<< "Turn send indication with no data for non-UDP transport.  Dropping.");
         return;
      }
      // !SLG! TODO - implement TCP relays
      assert(false);
   }
}

void 
TurnAllocation::sendDataToClient(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer> data)
{
   // Find RemotePeer
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(peerAddress);
   if(!remotePeer)
   {
      WarningLog(<< "sendDataToClient RemotePeer info not found - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" <<  mRequestedTuple << " peerAddress=" << peerAddress);
      return;
   }

   // Use DataInd if channel is not yet confirmed
   bool useDataInd = !remotePeer->isServerToClientChannelConfirmed();

   // If channel number is not allocated yet, allocate one
   if(remotePeer->getServerToClientChannel() == 0)
   {
      remotePeer->setServerToClientChannel(mChannelManager.getNextChannelNumber());
      if(mKey.getClientLocalTuple().getTransportType() != StunTuple::UDP)
      {
         // Set channel to confirmed for TCP and TLS transports
         remotePeer->setServerToClientChannelConfirmed();
      }
      // Add to lookup map
      mChannelManager.addRemotePeerServerToClientChannelLookup(remotePeer);
   }

   DebugLog(<< "TurnAllocation sendDataToClient: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
               mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " peer=" << peerAddress << 
               " channelNumber=" << (int)remotePeer->getServerToClientChannel());

   if(useDataInd)
   {
      StunMessage dataInd;
      dataInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnDataMethod);
      dataInd.mHasTurnPeerAddress = true;
      StunMessage::setStunAtrAddressFromTuple(dataInd.mTurnPeerAddress, peerAddress);
      dataInd.mHasTurnChannelNumber = true;
      dataInd.mTurnChannelNumber = remotePeer->getServerToClientChannel();
      dataInd.setTurnData(data->data(), (unsigned int)data->size());

      // send DataInd to local client
      unsigned int bufferSize = (unsigned int)data->size() + 8 /* Stun Header */ + 36 /* Remote Address (v6) */ +8 /* Channel Number */ + 8 /* TurnData Header + potential pad */ + 4 /* Turn Frame size */;
      boost::shared_ptr<DataBuffer> buffer = AsyncSocketBase::allocateBuffer(bufferSize);
      unsigned int size = dataInd.stunEncodeFramedMessage((char*)buffer->data(), bufferSize);
      buffer->truncate(size);  // Set size to proper size
      mLocalTurnSocket->doSend(mKey.getClientRemoteTuple(), buffer);
   }
   else
   {
      // send data to local client
      mLocalTurnSocket->doSend(mKey.getClientRemoteTuple(), remotePeer->getServerToClientChannel(), data);
   }
}

void TurnAllocation::serverToClientChannelConfirmed(unsigned short channelNumber,  const StunTuple& peerAddress)
{
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByServerToClientChannel(channelNumber);
   if(remotePeer)
   {
      if(remotePeer->getPeerTuple() == peerAddress)
      {
         remotePeer->setServerToClientChannelConfirmed();
      }
      else
      {
         WarningLog(<< "serverToClientChannelConfirmed bad peer address - discarding confirmed indication: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << channelNumber << " peerAddress=" << peerAddress);
      }
   }
   else
   {
      WarningLog(<< "serverToClientChannelConfirmed bad channel number - discarding confirmed indication: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
         mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << channelNumber);
   }
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

