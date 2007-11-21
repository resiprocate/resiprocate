#ifndef TURNALLOCATION_HXX
#define TURNALLOCATION_HXX

#include <map>
#include <boost/noncopyable.hpp>
#include <asio.hpp>

#include "StunTuple.hxx"
#include "StunAuth.hxx"
#include "TurnAllocationKey.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include "ChannelManager.hxx"

namespace reTurn {

class TurnPermission;
class TurnManager;
class AsyncSocketBase;
class UdpRelayServer;

class TurnAllocation
  : public AsyncSocketBaseDestroyedHandler,
    private boost::noncopyable
{
public:
   explicit TurnAllocation(TurnManager& turnManager,
                           AsyncSocketBase* localTurnSocket,
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

   // checks if the permission exists or not - also checks for expired
   // permissions
   bool existsPermission(const asio::ip::address& address);

   // create Permission if not present, otherwise refresh permission timer
   void refreshPermission(const asio::ip::address& address);

   // this get's called when being notified that the socket that the allocation came from
   // has been destroyed
   void onSocketDestroyed();

   // Used when framed data is received from client, to forward data to peer
   void sendDataToPeer(unsigned short channelNumber, resip::SharedPtr<resip::Data> data, bool framed);
   // Used when Send Indication is received from client, to forward data to peer
   void sendDataToPeer(unsigned short channelNumber, const StunTuple& peerAddress, resip::SharedPtr<resip::Data> data, bool framed);  
   // Used when Data is received from peer, to forward data to client
   void sendDataToClient(const StunTuple& peerAddress, resip::SharedPtr<resip::Data> data); 

   // Called when a ChannelConfirmed Indication is received
   void serverToClientChannelConfirmed(unsigned short channelNumber, const StunTuple& peerAddress);

   const StunTuple& getRequestedTuple() const { return mRequestedTuple; }
   time_t getExpires() const { return mExpires; }
   const StunAuth& getClientAuth() const { return mClientAuth; }

private:
   // Used when there is any data to send to the peer, after channel has been identified
   void sendDataToPeer(const StunTuple& peerAddress, resip::SharedPtr<resip::Data> data, bool framed);  

   TurnAllocationKey mKey;  // contains ClientLocalTuple and clientRemoteTuple
   StunAuth  mClientAuth;
   StunTuple mRequestedTuple;
   UInt32    mRequestedPortProps;
   UInt32    mRequestedTransport;
   asio::ip::address mRequestedIpAddress;

   time_t    mExpires;
   //unsigned int mBandwidth; // future use

   typedef std::map<asio::ip::address,TurnPermission*> TurnPermissionMap;
   TurnPermissionMap mTurnPermissionMap;

   TurnManager& mTurnManager;
   asio::deadline_timer mAllocationTimer;

   AsyncSocketBase* mLocalTurnSocket;
   boost::shared_ptr<UdpRelayServer> mUdpRelayServer;

   ChannelManager mChannelManager;
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

