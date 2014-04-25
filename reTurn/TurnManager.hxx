#ifndef TURNMANAGER_HXX
#define TURNMANAGER_HXX

#include <map>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include "ReTurnConfig.hxx"
#include "StunTuple.hxx"

namespace reTurn {

class TurnManager
{
public:
   explicit TurnManager(asio::io_service& ioService, const ReTurnConfig& config);  // ioService used to start timers
   ~TurnManager();

   asio::io_service& getIOService() { return mIOService; }

   unsigned short allocateAnyPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPort(StunTuple::TransportType transport);
   unsigned short allocateOddPort(StunTuple::TransportType transport);
   unsigned short allocateEvenPortPair(StunTuple::TransportType transport);
   bool allocatePort(StunTuple::TransportType transport, unsigned short port, bool reserved = false);
   void deallocatePort(StunTuple::TransportType transport, unsigned short port);

   const ReTurnConfig& getConfig() { return mConfig; }

private:

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
   const ReTurnConfig& mConfig;
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
