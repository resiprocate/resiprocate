#include <rutil/Lock.hxx>

#include "TurnManager.hxx"
#include "TurnAllocation.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

TurnManager::TurnManager(asio::io_service& ioService, const ReTurnConfig& config) : 
   mLastAllocatedUdpPort(config.mAllocationPortRangeMin-1),
   mLastAllocatedTcpPort(config.mAllocationPortRangeMin-1),
   mIOService(ioService),
   mConfig(config)
{
   // Initialize Allocation Ports
   for(unsigned short i = config.mAllocationPortRangeMin; i <= config.mAllocationPortRangeMax && i != 0; i++) // i != 0 catches case where we increment 65535 (as an unsigned short)
   {
      mUdpAllocationPorts[i] = PortStateUnallocated;
      mTcpAllocationPorts[i] = PortStateUnallocated;
   }
}

TurnManager::~TurnManager()
{
   InfoLog(<< "Turn Manager destroyed.");
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
   while(startPortToCheck % 2 != 0)
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

// Note:  This is not used, since requesting an odd port was removed
unsigned short 
TurnManager::allocateOddPort(StunTuple::TransportType transport)
{
   PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
   unsigned short startPortToCheck = advanceLastAllocatedPort(transport);
   // Ensure start port is odd
   while(startPortToCheck % 2 != 1)
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
   // Ensure start port is even and that start port + 1 is in range
   while(startPortToCheck % 2 != 0 ||
         startPortToCheck + 1 == 0 ||
         startPortToCheck + 1 > mConfig.mAllocationPortRangeMax )
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
TurnManager::allocatePort(StunTuple::TransportType transport, unsigned short port, bool reserved)
{
   if(port >= mConfig.mAllocationPortRangeMin && port <= mConfig.mAllocationPortRangeMax)
   {
      PortAllocationMap& portAllocationMap = getPortAllocationMap(transport);
      if(reserved)
      {
         if(portAllocationMap[port] == PortStateReserved)
         {
            portAllocationMap[port] = PortStateAllocated;
            return true;
         }
      }
      else
      {
         if(portAllocationMap[port] == PortStateUnallocated)  
         {
            portAllocationMap[port] = PortStateAllocated;
            return true;
         }
      }
   }
   return false;
}

void 
TurnManager::deallocatePort(StunTuple::TransportType transport, unsigned short port)
{
   if(port >= mConfig.mAllocationPortRangeMin && port <= mConfig.mAllocationPortRangeMax)
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
      if(mLastAllocatedTcpPort > mConfig.mAllocationPortRangeMax) 
      {
         mLastAllocatedTcpPort = mConfig.mAllocationPortRangeMin+(mLastAllocatedTcpPort-mConfig.mAllocationPortRangeMax-1);
      }
      else if(mLastAllocatedTcpPort == 0 /* Wrap around */)
      {
         mLastAllocatedTcpPort = mConfig.mAllocationPortRangeMin;
      }
      return mLastAllocatedTcpPort;
   case StunTuple::UDP:
   default:
      mLastAllocatedUdpPort+=numToAdvance;
      if(mLastAllocatedUdpPort > mConfig.mAllocationPortRangeMax) 
      {
         mLastAllocatedUdpPort = mConfig.mAllocationPortRangeMin+(mLastAllocatedUdpPort-mConfig.mAllocationPortRangeMax-1);
      }
      else if(mLastAllocatedUdpPort == 0 /* Wrap around */)
      {
         mLastAllocatedUdpPort = mConfig.mAllocationPortRangeMin;
      }
      return mLastAllocatedUdpPort;
   }
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
