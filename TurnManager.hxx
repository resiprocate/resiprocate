#ifndef TURNMANAGER_HXX
#define TURNMANAGER_HXX

#include <map>
#include <asio.hpp>
#include "TurnAllocationKey.hxx"
#include "StunTuple.hxx"

namespace reTurn {

class TurnAllocation;

class TurnManager
{
public:
   explicit TurnManager(asio::io_service& ioService);  // ioService used to start timers

   void addTurnAllocation(TurnAllocation* turnAllocation);
   void removeTurnAllocation(const TurnAllocationKey& turnAllocationKey);

   void startAllocationExpirationTimer(const TurnAllocationKey& turnAllocationKey, int lifetime);

   TurnAllocation* findTurnAllocation(const TurnAllocationKey& turnAllocationKey);
   TurnAllocation* findTurnAllocation(const StunTuple& requestedTuple);

   asio::io_service& getIOService() { return mIOService; }

   void allocationExpired(const asio::error_code& e, const TurnAllocationKey& turnAllocationKey);

   unsigned short allocateAnyPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPort(StunTuple::TransportType transport);
   unsigned short allocateOddPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPortPair(StunTuple::TransportType transport);
   bool allocatePort(StunTuple::TransportType transport, unsigned short port);
   void deallocatePort(StunTuple::TransportType transport, unsigned short port);

private:
   typedef std::map<TurnAllocationKey, TurnAllocation*> TurnAllocationMap;  // .slg. consider using hash table
   TurnAllocationMap mTurnAllocationMap;
 
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

   asio::io_service& mIOService;
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

