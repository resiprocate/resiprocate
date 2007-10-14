#ifndef TURNALLOCATION_HXX
#define TURNALLOCATION_HXX

#include <map>
#include <boost/noncopyable.hpp>
#include <asio.hpp>

#include "StunTuple.hxx"
#include "StunAuth.hxx"
#include "TurnAllocationKey.hxx"
#include "TurnTransportHandler.hxx"

namespace reTurn {

class TurnPermission;
class TurnManager;
class UdpRelayServer;
class TurnTransportBase;

class TurnAllocation
  : public TurnTransportHandler,
    private boost::noncopyable
{
public:
   explicit TurnAllocation(TurnManager& turnManager,
                           TurnTransportBase* localTurnTransport,
                           const StunTuple& clientLocalTuple, 
                           const StunTuple& clientRemoteTuple,
                           const StunAuth& clientAuth, 
                           const StunTuple& requestedTuple, 
                           unsigned int lifetime=0,
                           UInt32 requestedPortProps=0,      // requested props are only store for comparison later
                           UInt32 requestedTransport=StunTuple::None, 
                           asio::ip::address* requestedIpAddress=0);
   ~TurnAllocation();

   const TurnAllocationKey& getKey() { return mKey; }
   void  refresh(unsigned int lifetime);  // update expiration time

   void setActiveDestination(const StunTuple& activeDestination);
   bool isActiveDestinationSet() { return mActiveDestinationSet; }
   void clearActiveDestination();

   // checks if the permission exists or not - also checks for expired
   // permissions
   bool existsPermission(const asio::ip::address& address);

   // create Permission if not present, otherwise refresh permission timer
   void refreshPermission(const asio::ip::address& address);

   // this get's called when being notified that the transport that the allocation came from
   // has been destroyed
   void onTransportDestroyed();

   void sendDataToRemote(const resip::Data& data);
   void sendDataToRemote(const StunTuple& remoteAddress, const resip::Data& data);
   void sendDataToLocal(const StunTuple& fromAddress, const resip::Data& data);

   // !slg! todo - should be private with accessors
   TurnAllocationKey mKey;  // contains ClientLocalTuple and clientRemoteTuple
   StunAuth  mClientAuth;
   StunTuple mRequestedTuple;
   UInt32    mRequestedPortProps;
   UInt32    mRequestedTransport;
   asio::ip::address mRequestedIpAddress;

   time_t    mExpires;
   //unsigned int mBandwidth; // future use

   StunTuple mActiveDestination;
   bool      mActiveDestinationSet;

private:
   typedef std::map<asio::ip::address,TurnPermission*> TurnPermissionMap;
   TurnPermissionMap mTurnPermissionMap;

   TurnManager& mTurnManager;
   asio::deadline_timer mAllocationTimer;

   TurnTransportBase* mLocalTurnTransport;
   UdpRelayServer* mUdpRelayServer;
};

} 

#endif


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

