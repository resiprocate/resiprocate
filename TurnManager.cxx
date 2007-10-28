#include <rutil/Lock.hxx>

#include "TurnManager.hxx"
#include "TurnAllocation.hxx"

using namespace std;

namespace reTurn {

#define PORT_RANGE_MIN 50000  // must be even
#define PORT_RANGE_MAX 50999  // must be odd

TurnManager::TurnManager(asio::io_service& ioService) : 
   mLastAllocatedUdpPort(PORT_RANGE_MIN-1),
   mLastAllocatedTcpPort(PORT_RANGE_MIN-1),
   mIOService(ioService)
{
   // Initialize Allocation Ports
   for(unsigned short i = PORT_RANGE_MIN; i <= PORT_RANGE_MAX; i++)
   {
      mUdpAllocationPorts[i] = PortStateUnallocated;
      mTcpAllocationPorts[i] = PortStateUnallocated;
   }
}

void 
TurnManager::addTurnAllocation(TurnAllocation* turnAllocation)
{
   assert(findTurnAllocation(turnAllocation->getKey()) == 0);    // ?slg? what if already exists - delete?  assert doesn't exist for now

   mTurnAllocationMap[turnAllocation->getKey()] = turnAllocation;
}

void 
TurnManager::removeTurnAllocation(const TurnAllocationKey& turnAllocationKey)
{
   TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
   if(it != mTurnAllocationMap.end())
   {
      delete it->second;
      mTurnAllocationMap.erase(it);
   }
}

TurnAllocation* 
TurnManager::findTurnAllocation(const TurnAllocationKey& turnAllocationKey)
{
   TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
   if(it != mTurnAllocationMap.end())
   {
      return it->second;
   }
   return 0;
}

TurnAllocation* 
TurnManager::findTurnAllocation(const StunTuple& requestedTuple)
{
   TurnAllocationMap::iterator it;
   for(it = mTurnAllocationMap.begin(); it != mTurnAllocationMap.end(); it++)
   {
      if(it->second->mRequestedTuple == requestedTuple)
      {
         return it->second;
      }
   }
   return 0;
}

void 
TurnManager::allocationExpired(const asio::error_code& e, const TurnAllocationKey& turnAllocationKey)
{
   if (e != asio::error::operation_aborted)  // Note: nothing currently stops timers
   {
      // Timer was not cancelled, take necessary action.
      cout << "Turn Allocation Expired! clientLocal=" << turnAllocationKey.getClientLocalTuple() << " clientRemote=" << turnAllocationKey.getClientRemoteTuple() << endl;

      TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
      if(it != mTurnAllocationMap.end())
      {
         if(time(0) >= it->second->mExpires)
         {
            delete it->second;
            mTurnAllocationMap.erase(it);
         }
      }
   }
}

unsigned short 
TurnManager::allocateAnyPort(StunTuple::TransportType transport)
{
   PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
   unsigned short startPortToCheck = advanceLastAllocatedPort(transport);
   unsigned short portToCheck = startPortToCheck;
   while(portAllocationMap[portToCheck] != PortStateUnallocated)
   {
      portToCheck = advanceLastAllocatedPort(transport);
      if(portToCheck == startPortToCheck) return 0;  // If we checked all available ports and none found - then return 0
   }
   portAllocationMap[portToCheck] = PortStateAllocated;
   return portToCheck;
}

unsigned short 
TurnManager::allocateEvenPort(StunTuple::TransportType transport)
{
   PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
   unsigned short startPortToCheck = advanceLastAllocatedPort(transport);
   // Ensure start port is even
   if(startPortToCheck % 2 != 0)
   {
      startPortToCheck = advanceLastAllocatedPort(transport);
   }
   unsigned short portToCheck = startPortToCheck;
   while(portAllocationMap[portToCheck] != PortStateUnallocated)
   {
      portToCheck = advanceLastAllocatedPort(transport, 2);
      if(portToCheck == startPortToCheck) return 0;  // If we checked all available ports and none found - then return 0
   }
   portAllocationMap[portToCheck] = PortStateAllocated;
   return portToCheck;
}

unsigned short 
TurnManager::allocateOddPort(StunTuple::TransportType transport)
{
   PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
   unsigned short startPortToCheck = advanceLastAllocatedPort(transport);
   // Ensure start port is odd
   if(startPortToCheck % 2 != 1)
   {
      startPortToCheck = advanceLastAllocatedPort(transport);
   }
   unsigned short portToCheck = startPortToCheck;
   while(portAllocationMap[portToCheck] != PortStateUnallocated)
   {
      portToCheck = advanceLastAllocatedPort(transport, 2);
      if(portToCheck == startPortToCheck) return 0;  // If we checked all available ports and none found - then return 0
   }
   portAllocationMap[portToCheck] = PortStateAllocated;
   return portToCheck;
}

unsigned short 
TurnManager::allocateEvenPortPair(StunTuple::TransportType transport)
{
   PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
   unsigned short startPortToCheck = advanceLastAllocatedPort(transport);
   // Ensure start port is even
   if(startPortToCheck % 2 != 0)
   {
      startPortToCheck = advanceLastAllocatedPort(transport);
   }
   unsigned short portToCheck = startPortToCheck;
   while(portAllocationMap[portToCheck] != PortStateUnallocated || portAllocationMap[portToCheck+1] != PortStateUnallocated)
   {
      portToCheck = advanceLastAllocatedPort(transport, 2);
      if(portToCheck == startPortToCheck) return 0;  // If we checked all available ports and none found - then return 0
   }
   portAllocationMap[portToCheck] = PortStateAllocated;
   portAllocationMap[portToCheck+1] = PortStateReserved;
   return portToCheck;
}

bool 
TurnManager::allocatePort(StunTuple::TransportType transport, unsigned short port)
{
   if(port >= PORT_RANGE_MIN && port <= PORT_RANGE_MAX)
   {
      PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
      if(portAllocationMap[port] != PortStateAllocated)  // Allow reserved state to be allocated with specific port request
      {
         portAllocationMap[port] = PortStateAllocated;
         return true;
      }
   }
   return false;
}

void 
TurnManager::deallocatePort(StunTuple::TransportType transport, unsigned short port)
{
   if(port >= PORT_RANGE_MIN && port <= PORT_RANGE_MAX)
   {
      PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
      portAllocationMap[port] = PortStateUnallocated;

      // If port is even - check if next higher port is reserved - if so unallocate it
      if(port % 2 == 0 && portAllocationMap[port+1] == PortStateReserved)
      {
         portAllocationMap[port+1] = PortStateUnallocated;
      }
   }
}

TurnManager::PortAllocationMap& 
TurnManager::getPortAllocationMap(StunTuple::TransportType transport)
{
   switch(transport)
   {
   case StunTuple::TCP:
   case StunTuple::TLS:
      return mTcpAllocationPorts;
   case StunTuple::UDP:
   default:
      return mUdpAllocationPorts;
   }
}

unsigned short 
TurnManager::advanceLastAllocatedPort(StunTuple::TransportType transport, unsigned int numToAdvance)
{
   switch(transport)
   {
   case StunTuple::TCP:
   case StunTuple::TLS:
      mLastAllocatedTcpPort+=numToAdvance;
      if(mLastAllocatedTcpPort > PORT_RANGE_MAX) 
      {
         mLastAllocatedTcpPort = PORT_RANGE_MIN+(mLastAllocatedTcpPort-PORT_RANGE_MAX-1);
      }
      return mLastAllocatedTcpPort;
   case StunTuple::UDP:
   default:
      mLastAllocatedUdpPort+=numToAdvance;
      if(mLastAllocatedUdpPort > PORT_RANGE_MAX) 
      {
         mLastAllocatedUdpPort = PORT_RANGE_MIN+(mLastAllocatedUdpPort-PORT_RANGE_MAX-1);
      }
      return mLastAllocatedUdpPort;
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

