#ifndef TURNALLOCATION_HXX
#define TURNALLOCATION_HXX

#include <map>
#include <boost/noncopyable.hpp>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif

#include "StunTuple.hxx"
#include "StunAuth.hxx"
#include "TurnAllocationKey.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include "DataBuffer.hxx"
#include "ChannelManager.hxx"

namespace reTurn {

class TurnPermission;
class TurnManager;
class TurnAllocationManager;
class AsyncSocketBase;
class UdpRelayServer;

class TurnAllocation
  : public AsyncSocketBaseHandler,
    private boost::noncopyable
{
public:
   explicit TurnAllocation(TurnManager& turnManager,
                           TurnAllocationManager& turnAllocationManager,
                           AsyncSocketBase* localTurnSocket,
                           const StunTuple& clientLocalTuple, 
                           const StunTuple& clientRemoteTuple,
                           const StunAuth& clientAuth, 
                           const StunTuple& requestedTuple, 
                           unsigned int lifetime);
   ~TurnAllocation();

   bool startRelay();
   void stopRelay();

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
   void sendDataToPeer(unsigned short channelNumber, boost::shared_ptr<DataBuffer>& data, bool isFramed);
   // Used when Send Indication is received from client, to forward data to peer
   void sendDataToPeer(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer>& data, bool isFramed);  
   // Used when Data is received from peer, to forward data to client
   void sendDataToClient(const StunTuple& peerAddress, boost::shared_ptr<DataBuffer>& data); 

   // Called when a ChannelBind Request is received
   bool addChannelBinding(const StunTuple& peerAddress, unsigned short channelNumber);

   const StunTuple& getRequestedTuple() const { return mRequestedTuple; }
   time_t getExpires() const { return mExpires; }
   const StunAuth& getClientAuth() const { return mClientAuth; }

private:
   TurnAllocationKey mKey;  // contains ClientLocalTuple and clientRemoteTuple
   StunAuth  mClientAuth;
   StunTuple mRequestedTuple;

   time_t    mExpires;
   //unsigned int mBandwidth; // future use

   typedef std::map<asio::ip::address,TurnPermission*> TurnPermissionMap;
   TurnPermissionMap mTurnPermissionMap;

   TurnManager& mTurnManager;
   TurnAllocationManager& mTurnAllocationManager;
   asio::deadline_timer mAllocationTimer;

   AsyncSocketBase* mLocalTurnSocket;
   boost::shared_ptr<UdpRelayServer> mUdpRelayServer;

   ChannelManager mChannelManager;

   // Flags to control logging on Data channel/relay.  Used so that errors only print at Warning level once
   bool mBadChannelErrorLogged;
   bool mNoPermissionToPeerLogged;
   bool mNoPermissionFromPeerLogged;
};

} 

#endif


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
