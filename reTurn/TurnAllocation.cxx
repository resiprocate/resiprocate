#include <boost/bind.hpp>

#include "TurnAllocation.hxx"
#include "TurnAllocationManager.hxx"
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

#define TURN_PERMISSION_LIFETIME_SECONDS 300   // 5 minuntes
//#define TURN_PERMISSION_LIFETIME_SECONDS 30   // TESTING only

namespace reTurn {

TurnAllocation::TurnAllocation(TurnManager& turnManager,
                               TurnAllocationManager& turnAllocationManager,
                               AsyncSocketBase* localTurnSocket,
                               const StunTuple& clientLocalTuple, 
                               const StunTuple& clientRemoteTuple,
                               const StunAuth& clientAuth, 
                               const StunTuple& requestedTuple, 
                               unsigned int lifetime) :
   mKey(clientLocalTuple, clientRemoteTuple),
   mClientAuth(clientAuth),
   mRequestedTuple(requestedTuple),
   mTurnManager(turnManager),
   mTurnAllocationManager(turnAllocationManager),
   mAllocationTimer(turnManager.getIOService()),
   mLocalTurnSocket(localTurnSocket),
   mBadChannelErrorLogged(false),
   mNoPermissionToPeerLogged(false),
   mNoPermissionFromPeerLogged(false)
{
   InfoLog(<< "TurnAllocation created: clientLocal=" << clientLocalTuple << " clientRemote=" << 
           clientRemoteTuple << " allocation=" << requestedTuple << " lifetime=" << lifetime);

   refresh(lifetime);

   // Register for Turn Transport onDestroyed notification
   mLocalTurnSocket->registerAsyncSocketBaseHandler(this);
}

TurnAllocation::~TurnAllocation()
{
   InfoLog(<< "TurnAllocation destroyed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple);

   stopRelay();

   // Deallocate Port
   mTurnManager.deallocatePort(mRequestedTuple.getTransportType(), mRequestedTuple.getPort());

   // Cleanup Permission Memory
   TurnPermissionMap::iterator it;   
   for(it = mTurnPermissionMap.begin(); it != mTurnPermissionMap.end(); it++)
   {
      delete it->second;
   }
   
   // Unregister for TurnTransport notifications
   if(mLocalTurnSocket)
   {
      mLocalTurnSocket->registerAsyncSocketBaseHandler(0);
   }
}

bool 
TurnAllocation::startRelay()
{
   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      mUdpRelayServer.reset(new UdpRelayServer(mTurnManager.getIOService(), *this));
      if(!mUdpRelayServer->startReceiving())
      {
         stopRelay();  // Ensure allocation timer is stopped
         return false;
      }
      return true;
   }
   else
   {
      ErrLog(<< "Only UDP relay's are currently implemented!");
      resip_assert(false);
      stopRelay();  // Ensure allocation timer is stopped
      return false;
   }
}

void 
TurnAllocation::stopRelay()
{
   // Stop and detach Relay Server
   if(mUdpRelayServer.get())
   {
      mUdpRelayServer->stop();
      mUdpRelayServer.reset();
   }
   mAllocationTimer.cancel();
}

void  
TurnAllocation::refresh(unsigned int lifetime)  // update expiration time
{
   InfoLog(<< "TurnAllocation refreshed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " lifetime=" << lifetime);

   mExpires = time(0) + lifetime;

   // start timer
   mAllocationTimer.expires_from_now(boost::posix_time::seconds(lifetime));
   mAllocationTimer.async_wait(boost::bind(&TurnAllocationManager::allocationExpired, &mTurnAllocationManager, asio::placeholders::error, mKey));
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
            mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " exipred address=" << it->first.to_string());
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
      mTurnPermissionMap[address] = new TurnPermission(address, TURN_PERMISSION_LIFETIME_SECONDS);  
      InfoLog(<< "Permission for " << address.to_string() << " created: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
              mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple);
   }
   else
   {
      turnPermission->refresh();
      InfoLog(<< "Permission for " << address.to_string() << " refreshed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
              mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple);
   }
}

void 
TurnAllocation::onSocketDestroyed()
{
   mTurnAllocationManager.removeTurnAllocation(mKey);   // will delete this
}

void 
TurnAllocation::sendDataToPeer(unsigned short channelNumber, boost::shared_ptr<DataBuffer>& data, bool isFramed)
{
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByChannel(channelNumber);
   if(remotePeer)
   {
      // channel found - send Data
      sendDataToPeer(remotePeer->getPeerTuple(), data, isFramed);
   }
   else
   {
      // Log at Warning level first time only
      if(mBadChannelErrorLogged)
      {
         DebugLog(<< "sendDataToPeer bad channel number - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " channelNumber=" << channelNumber);
      }
      else
      {
         mBadChannelErrorLogged = true;
         WarningLog(<< "sendDataToPeer bad channel number - discarding data: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
            mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " channelNumber=" << channelNumber);
      }
   }
}

void 
TurnAllocation::sendDataToPeer(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer>& data, bool isFramed)
{
   DebugLog(<< "TurnAllocation sendDataToPeer: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " peerAddress=" << peerAddress);

   // Ensure permission exists
   if(!existsPermission(peerAddress.getAddress()))
   {
      // Log at Warning level first time only
      if(mNoPermissionToPeerLogged)
      {
         DebugLog(<< "Turn send indication for destination=" << peerAddress.getAddress() << ", but no permission installed.  Dropping.");
      }
      else
      {
         mNoPermissionToPeerLogged = true;
         WarningLog(<< "Turn send indication for destination=" << peerAddress.getAddress() << ", but no permission installed.  Dropping.");
      }
      return;
   }
   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      resip_assert(mUdpRelayServer);
      mUdpRelayServer->doSend(peerAddress, data, isFramed ? 4 /* bufferStartPos is 4 so that framing is skipped */ : 0);
   }
   else
   {
      if(data->size() <=4)
      {
         WarningLog(<< "Turn send indication with no data for non-UDP transport.  Dropping.");
         return;
      }
      // !SLG! TODO - implement TCP relays
      resip_assert(false);
   }
}

void 
TurnAllocation::sendDataToClient(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer>& data)
{
   // See if a permission exists
   if(!existsPermission(peerAddress.getAddress()))
   {
      // Log at Warning level first time only
      if(mNoPermissionFromPeerLogged)
      {
         DebugLog(<< "Data received from peer=" << peerAddress << ", but no permission installed.  Dropping.");
      }
      else
      {
         mNoPermissionFromPeerLogged = true;
         WarningLog(<< "Data received from peer=" << peerAddress << ", but no permission installed.  Dropping.");
      }
      return;
   }
   // See if a channel binding exists - if so, use it
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(peerAddress);
   if(remotePeer)
   {
      // send data to local client
      mLocalTurnSocket->doSend(mKey.getClientRemoteTuple(), remotePeer->getChannel(), data);

      DebugLog(<< "TurnAllocation sendDataToClient: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
                  mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " peer=" << peerAddress << 
                  " channelNumber=" << (int)remotePeer->getChannel());
   }
   else
   {
      // No Channel Binding - use DataInd
      StunMessage dataInd;
      dataInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnDataMethod);
      dataInd.mCntTurnXorPeerAddress = 1;
      StunMessage::setStunAtrAddressFromTuple(dataInd.mTurnXorPeerAddress[0], peerAddress);
      dataInd.setTurnData(data->data(), (unsigned int)data->size());

      // send DataInd to local client
      unsigned int bufferSize = (unsigned int)data->size() + 8 /* Stun Header */ + 36 /* Remote Address (v6) */ + 8 /* TurnData Header + potential pad */;
      boost::shared_ptr<DataBuffer> buffer = AsyncSocketBase::allocateBuffer(bufferSize);
      unsigned int size = dataInd.stunEncodeMessage((char*)buffer->data(), bufferSize);
      buffer->truncate(size);  // Set size to proper size
      mLocalTurnSocket->doSend(mKey.getClientRemoteTuple(), buffer);

      DebugLog(<< "TurnAllocation sendDataToClient: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
                  mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " peer=" << peerAddress << 
                  " using DataInd.");
   }
}

bool 
TurnAllocation::addChannelBinding(const StunTuple& peerAddress, unsigned short channelNumber)
{
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByChannel(channelNumber);
   if(remotePeer)
   {
      if(remotePeer->getPeerTuple() != peerAddress)
      {
         WarningLog(<< "addChannelBinding failed since channel is already in use: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
                    mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " channelNumber=" << channelNumber << " peerAddress=" << peerAddress);
         return false;
      }
      // refresh channel binding lifetime
      remotePeer->refresh();

      InfoLog(<< "Channel " << channelNumber << " binding to " << peerAddress << " refreshed: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
              mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple);
   }
   else
   {
      remotePeer = mChannelManager.findRemotePeerByPeerAddress(peerAddress);
      if(remotePeer)
      {
         WarningLog(<< "addChannelBinding failed since peerAddress is alredy in use: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
                    mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple << " channelNumber=" << channelNumber << " peerAddress=" << peerAddress);
         return false;
      }
      mChannelManager.createChannelBinding(peerAddress, channelNumber);

      InfoLog(<< "Channel " << channelNumber << " binding to " << peerAddress << " created: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
              mKey.getClientRemoteTuple() << " allocation=" << mRequestedTuple);
   }

   // Add or refresh permission
   refreshPermission(peerAddress.getAddress());

   return true;
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
