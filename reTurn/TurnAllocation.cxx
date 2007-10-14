#include <boost/bind.hpp>

#include "TurnAllocation.hxx"
#include "TurnManager.hxx"
#include "TurnPermission.hxx"
#include "TurnTransportBase.hxx"
#include "UdpRelayServer.hxx"

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
   mTurnManager(turnManager),
   mKey(clientLocalTuple, clientRemoteTuple),
   mClientAuth(clientAuth),
   mRequestedTuple(requestedTuple),
   mRequestedPortProps(requestedPortProps),
   mRequestedTransport(requestedTransport),
   mActiveDestinationSet(false),
   mAllocationTimer(turnManager.getIOService()),
   mLocalTurnTransport(localTurnTransport),
   mUdpRelayServer(0)
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
   TurnPermissionMap::iterator it;   
   for(it = mTurnPermissionMap.begin(); it != mTurnPermissionMap.end(); it++)
   {
      delete it->second;
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
TurnAllocation::setActiveDestination(const StunTuple& activeDestination)
{
   cout << "TurnAllocation active destination set: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " destination=" << activeDestination << endl;

   mActiveDestination = activeDestination;
   mActiveDestinationSet = true;

   // add permission if it does not exist
   refreshPermission(activeDestination.getAddress());
}

void 
TurnAllocation::clearActiveDestination()
{
   cout << "TurnAllocation active destination cleared: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << endl;

   mActiveDestinationSet = false;
}

void 
TurnAllocation::sendDataToRemote(const resip::Data& data)
{
   assert(mActiveDestinationSet);
   sendDataToRemote(mActiveDestination, data);
}

void 
TurnAllocation::sendDataToRemote(const StunTuple& remoteAddress, const Data& data)
{
   cout << "TurnAllocation sendRemoteData: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " destination=" << remoteAddress << " data=" << data << endl;

   // add permission if it does not exist
   refreshPermission(remoteAddress.getAddress());

   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      assert(mUdpRelayServer);
      mUdpRelayServer->sendTurnData(remoteAddress, data.data(), (unsigned int)data.size());
   }
}

void 
TurnAllocation::sendDataToLocal(const StunTuple& fromAddress, const Data& data)
{
   cout << "TurnAllocation sendDataToLocal: clientLocal=" << mKey.getClientLocalTuple() << " clientRemote=" << 
           mKey.getClientRemoteTuple() << " requested=" << mRequestedTuple << " from=" << fromAddress << " data=" << data << endl;

   bool useDataInd = false;

   // If UDP and Data contains StunMagic cookie then encapsulate in DataInd
   if(mRequestedTuple.getTransportType() == StunTuple::UDP)
   {
      if(data.size() > 8)  // StunMagicCookie will be in bytes 4-8
      {
         unsigned int magicCookie;
         memcpy(&magicCookie, data.data()+4, sizeof(magicCookie));
         //magicCookie = ntohl(magicCookie);
         if(magicCookie == StunMessage::StunMagicCookie)
         {
            useDataInd = true;
         }
      }
   }
   if(!useDataInd)
   {
      // If active destination is not set, or does not match this data then wrap in a DataInd
      if(!mActiveDestinationSet || mActiveDestination != fromAddress)
      {
         useDataInd = true;
      }
   }
   if(useDataInd)
   {
      StunMessage dataInd;
      dataInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnDataInd);
      dataInd.mHasTurnRemoteAddress = true;
      dataInd.mTurnRemoteAddress.port = fromAddress.getPort();
      if(fromAddress.getAddress().is_v6())
      {            
         dataInd.mTurnRemoteAddress.family = StunMessage::IPv6Family;
         memcpy(&dataInd.mTurnRemoteAddress.addr.ipv6, fromAddress.getAddress().to_v6().to_bytes().c_array(), sizeof(dataInd.mTurnRemoteAddress.addr.ipv6));
      }
      else
      {
         dataInd.mTurnRemoteAddress.family = StunMessage::IPv4Family;
         dataInd.mTurnRemoteAddress.addr.ipv4 = fromAddress.getAddress().to_v4().to_ulong();   
      }
      dataInd.setTurnData(data.data(), (unsigned int)data.size());
      dataInd.mHasFingerprint = true;

      // send DataInd to local client
      unsigned int bufferSize = (unsigned int)data.size() + 8 /* Stun Header */ + 36 /* Remote Address (v6) */ + 8 /* TurnData Header + potential pad */ + 8 /* Fingerprint */;
      Data buffer(bufferSize, Data::Preallocate);
      unsigned int size = dataInd.stunEncodeMessage((char*)buffer.data(), bufferSize);
      mLocalTurnTransport->sendTurnData(mKey.getClientRemoteTuple(), buffer.data(), size);
   }
   else
   {
      // send data to local client
      mLocalTurnTransport->sendTurnData(mKey.getClientRemoteTuple(), data.data(), (unsigned int)data.size());
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

