#include <boost/bind.hpp>

#include "TurnAllocation.hxx"
#include "TurnManager.hxx"
#include "TurnPermission.hxx"
#include "TurnTransportBase.hxx"
#include "UdpRelayServer.hxx"
#include "RemotePeer.hxx"

using namespace std;
using namespace resip;

#define TURN_PERMISSION_INACTIVITY_SECONDS 60   // .slg. move to configuration

namespace reTurn {

TurnAllocation::TurnAllocation(TurnManager& turnManager,
                               TurnTransportBase* localTurnTransport,
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
   mLocalTurnTransport(localTurnTransport),
   mUdpRelayServer(0),
   mNextServerToClientChannel(1)
{
   if(requestedIpAddress)
   {
      mRequestedIpAddress = *requestedIpAddress;
   }
   cout << "TurnAllocation created: clientLocal=" << clientLocalTuple << " clientRemote=" << 
           clientRemoteTuple << " requested=" << requestedTuple << " lifetime=" << lifetime << endl;
   refresh(lifetime);

   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      mUdpRelayServer = new UdpRelayServer(turnManager.getIOService(), *this);
   }

   // Register for Turn Transport onDestroyed notification
   mLocalTurnTransport->registerTurnTransportHandler(this);
}

TurnAllocation::~TurnAllocation()
{
   cout << "TurnAllocation destroyed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << endl;

   // Delete Relay Servers
   if(mUdpRelayServer) delete mUdpRelayServer;

   // Deallocate Port
   mTurnManager.deallocatePort(mRequestedTuple.getTransportType(), mRequestedTuple.getPort());

   // Cleanup Permission Memory
   {
      TurnPermissionMap::iterator it;   
      for(it = mTurnPermissionMap.begin(); it != mTurnPermissionMap.end(); it++)
      {
         delete it->second;
      }
   }

   // Cleanup RemotePeer Memory
   {
      TupleRemotePeerMap::iterator it;   
      for(it = mTupleRemotePeerMap.begin(); it != mTupleRemotePeerMap.end(); it++)
      {
         delete it->second;
      }
   }
   
   // Unregister for TurnTransport notifications
   mLocalTurnTransport->registerTurnTransportHandler(0);
}

void  
TurnAllocation::refresh(unsigned int lifetime)  // update expiration time
{
   cout << "TurnAllocation refreshed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " lifetime=" << lifetime << endl;

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
         cout << "TurnAllocation has expired permission: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " exipred address=" << it->first << endl;
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
TurnAllocation::onTransportDestroyed()
{
   mTurnManager.removeTurnAllocation(mKey);   // will delete this
}

void 
TurnAllocation::sendDataToPeer(unsigned char channelNumber, const resip::Data& data)
{
   ChannelRemotePeerMap::iterator it = mClientToServerChannelRemotePeerMap.find(channelNumber);
   if(it != mClientToServerChannelRemotePeerMap.end())
   {
      // channel found - send Data
      sendDataToPeer(it->second->getPeerTuple(), data);
   }
   else
   {
      cout << "sendDataToPeer bad channel number - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
         mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << (int)channelNumber << endl;
   }
}

void 
TurnAllocation::sendDataToPeer(unsigned char channelNumber, const StunTuple& peerAddress, const resip::Data& data)
{
   // Find RemotePeer
   TupleRemotePeerMap::iterator it = mTupleRemotePeerMap.find(peerAddress);
   if(it != mTupleRemotePeerMap.end())
   {
      // Found Remote Peer - ensure channel number matches
      if(channelNumber != it->second->getClientToServerChannel())
      {
         cout << "sendDataToPeer channel number mismatch - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" <<  mRequestedTuple << " oldChannelNumber=" << (int)it->second->getClientToServerChannel() <<
            " newChannelNumber=" << (int)channelNumber << endl;
         // drop message
         return;
      }
   }
   else
   {
      // Create New RemotePeer
      RemotePeer* remotePeer = new RemotePeer(peerAddress);
      remotePeer->setClientToServerChannel(channelNumber);
      remotePeer->setClientToServerChannelConfirmed();

      // Add RemoteAddress to two of the three maps
      mTupleRemotePeerMap[peerAddress] = remotePeer;
      mClientToServerChannelRemotePeerMap[channelNumber] = remotePeer;
   }

   if(mKey.getClientLocalTuple().getTransportType() == StunTuple::UDP)
   {
      // If UDP, then send TurnChannelConfirmationInd
      StunMessage channelConfirmationInd;
      channelConfirmationInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnChannelConfirmationInd);
      channelConfirmationInd.mHasTurnRemoteAddress = true;
      channelConfirmationInd.mTurnRemoteAddress.port = peerAddress.getPort();
      if(peerAddress.getAddress().is_v6())
      {            
         channelConfirmationInd.mTurnRemoteAddress.family = StunMessage::IPv6Family;
         memcpy(&channelConfirmationInd.mTurnRemoteAddress.addr.ipv6, peerAddress.getAddress().to_v6().to_bytes().c_array(), sizeof(channelConfirmationInd.mTurnRemoteAddress.addr.ipv6));
      }
      else
      {
         channelConfirmationInd.mTurnRemoteAddress.family = StunMessage::IPv4Family;
         channelConfirmationInd.mTurnRemoteAddress.addr.ipv4 = peerAddress.getAddress().to_v4().to_ulong();   
      }
      channelConfirmationInd.mHasTurnChannelNumber = true;
      channelConfirmationInd.mTurnChannelNumber = channelNumber;
      channelConfirmationInd.mHasFingerprint = true;

      // send channelConfirmationInd to local client
      unsigned int bufferSize = (unsigned int)data.size() + 8 /* Stun Header */ + 36 /* Remote Address (v6) */ + 8 /* TurnChannelNumber */ + 8 /* Fingerprint */ + 4 /* Turn Frame size */;
      Data buffer(bufferSize, Data::Preallocate);
      unsigned int size = channelConfirmationInd.stunEncodeFramedMessage((char*)buffer.data(), bufferSize);
      mLocalTurnTransport->sendTurnData(mKey.getClientRemoteTuple(), buffer.data(), size);
   }      

   sendDataToPeer(peerAddress, data);
}

void 
TurnAllocation::sendDataToPeer(const StunTuple& peerAddress, const resip::Data& data)
{
   cout << "TurnAllocation sendDataToPeer: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " peerAddress=" << peerAddress << " data=" << data << endl;

   // add permission if it does not exist
   refreshPermission(peerAddress.getAddress());

   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      assert(mUdpRelayServer);
      mUdpRelayServer->sendTurnData(peerAddress, data.data(), (unsigned int)data.size());
   }
   // !SLG! TODO - implement TCP relays
}

void 
TurnAllocation::sendDataToClient(const StunTuple& peerAddress, const Data& data)
{
   // Find RemotePeer
   TupleRemotePeerMap::iterator it = mTupleRemotePeerMap.find(peerAddress);
   if(it == mTupleRemotePeerMap.end())
   {
      cout << "sendDataToClient RemotePeer info not found - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" <<  mRequestedTuple << " peerAddress=" << peerAddress << endl;
      return;
   }

   // Use DataInd if channel is not yet confirmed
   bool useDataInd = !it->second->isServerToClientChannelConfirmed();

   // If channel number is not allocated yet, allocate one
   if(it->second->getServerToClientChannel() == 0)
   {
      it->second->setServerToClientChannel(mNextServerToClientChannel++);
      if(mKey.getClientLocalTuple().getTransportType() != StunTuple::UDP)
      {
         // Set channel to confirmed for TCP and TLS transports
         it->second->setServerToClientChannelConfirmed();
      }
   }

   cout << "TurnAllocation sendDataToClient: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
      mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " peer=" << peerAddress << " channelNumber=" << it->second->getServerToClientChannel() << " data=" << data << endl;

   if(useDataInd)
   {
      StunMessage dataInd;
      dataInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnDataInd);
      dataInd.mHasTurnRemoteAddress = true;
      dataInd.mTurnRemoteAddress.port = peerAddress.getPort();
      if(peerAddress.getAddress().is_v6())
      {            
         dataInd.mTurnRemoteAddress.family = StunMessage::IPv6Family;
         memcpy(&dataInd.mTurnRemoteAddress.addr.ipv6, peerAddress.getAddress().to_v6().to_bytes().c_array(), sizeof(dataInd.mTurnRemoteAddress.addr.ipv6));
      }
      else
      {
         dataInd.mTurnRemoteAddress.family = StunMessage::IPv4Family;
         dataInd.mTurnRemoteAddress.addr.ipv4 = peerAddress.getAddress().to_v4().to_ulong();   
      }
      dataInd.mHasTurnChannelNumber = true;
      dataInd.mTurnChannelNumber = it->second->getServerToClientChannel();
      dataInd.setTurnData(data.data(), (unsigned int)data.size());
      dataInd.mHasFingerprint = true;

      // send DataInd to local client
      unsigned int bufferSize = (unsigned int)data.size() + 8 /* Stun Header */ + 36 /* Remote Address (v6) */ +8 /* Channel Number */ + 8 /* TurnData Header + potential pad */ + 8 /* Fingerprint */  + 4 /* Turn Frame size */;
      Data buffer(bufferSize, Data::Preallocate);
      unsigned int size = dataInd.stunEncodeFramedMessage((char*)buffer.data(), bufferSize);
      mLocalTurnTransport->sendTurnData(mKey.getClientRemoteTuple(), buffer.data(), size);
   }
   else
   {
      // send data to local client
      mLocalTurnTransport->sendTurnFramedData(it->second->getServerToClientChannel(), mKey.getClientRemoteTuple(), data.data(), (unsigned int)data.size());
   }
}

void TurnAllocation::serverToClientChannelConfirmed(unsigned char channelNumber,  const StunTuple& peerAddress)
{
   ChannelRemotePeerMap::iterator it = mServerToClientChannelRemotePeerMap.find(channelNumber);
   if(it != mServerToClientChannelRemotePeerMap.end())
   {
      if(it->second->getPeerTuple() == peerAddress)
      {
         it->second->setServerToClientChannelConfirmed();
      }
      else
      {
         cout << "serverToClientChannelConfirmed bad peer address - discarding confirmed indication: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << (int)channelNumber << " peerAddress=" << peerAddress << endl;
      }
   }
   else
   {
      cout << "serverToClientChannelConfirmed bad channel number - discarding confirmed indication: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
         mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " channelNumber=" << (int)channelNumber << endl;
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

