#ifndef TURNMANAGER_HXX
#define TURNMANAGER_HXX

#include <map>
#include <set>
#include <asio.hpp>
#include <rutil/Data.hxx>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include "ReTurnConfig.hxx"
#include "StunTuple.hxx"

namespace reTurn {

class TurnManager
{
public:
   explicit TurnManager(asio::io_context& ioService, const ReTurnConfig& config);  // ioService used to start timers
   ~TurnManager();

   asio::io_context& getIOService() { return mIOService; }

   unsigned short allocateAnyPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPort(StunTuple::TransportType transport);
   unsigned short allocateOddPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPortPair(StunTuple::TransportType transport);
   bool allocatePort(StunTuple::TransportType transport, unsigned short port, bool reserved = false);
   void deallocatePort(StunTuple::TransportType transport, unsigned short port);

   const ReTurnConfig& getConfig() { return mConfig; }

   /// Allocations per user, over all transports: TurnAllocation adds and removes its own
   /// (MaxAllocationsPerUser)
   bool allocationQuotaReached(const resip::Data& username) const;
   void addAllocation(const resip::Data& username);
   void removeAllocation(const resip::Data& username);

   /// TCP and TLS connections per client IP address (MaxTcpConnectionsPerIp): addConnection
   /// returns false, counting nothing, if the address already has as many as allowed
   bool addConnection(const asio::ip::address& address);
   void removeConnection(const asio::ip::address& address);

private:
   std::map<resip::Data, unsigned long> mAllocationsPerUser;
   std::map<asio::ip::address, unsigned long> mConnectionsPerIp;
   std::set<asio::ip::address> mConnectionLimitLogged;  // addresses refused since they reached the limit


   typedef enum
   {
      PortStateUnallocated,
      PortStateAllocated,
      PortStateReserved
   } PortState;
   typedef std::map<unsigned short, PortState> PortAllocationMap;
   PortAllocationMap mUdpAllocationPorts;  // .slg. expand to be a map/hash table per ip address/interface
   PortAllocationMap mTcpAllocationPorts;
   unsigned short mLastAllocatedUdpPort;
   unsigned short mLastAllocatedTcpPort;
   PortAllocationMap& getPortAllocationMap(StunTuple::TransportType transport);
   unsigned short advanceLastAllocatedPort(StunTuple::TransportType transport, unsigned int numToAdvance = 1);

   asio::io_context& mIOService;
   const ReTurnConfig& mConfig;
};

} 

#endif


/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
