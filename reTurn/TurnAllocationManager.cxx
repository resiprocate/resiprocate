#include <rutil/Lock.hxx>

#include "TurnAllocationManager.hxx"
#include "TurnAllocation.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

TurnAllocationManager::TurnAllocationManager()
{
}

TurnAllocationManager::~TurnAllocationManager()
{
   TurnAllocationMap::iterator it = mTurnAllocationMap.begin();
   for(;it != mTurnAllocationMap.end();it++)
   {
      delete it->second;
   }

   InfoLog(<< "Turn Allocation Manager destroyed.");
}

void 
TurnAllocationManager::addTurnAllocation(TurnAllocation* turnAllocation)
{
   resip_assert(findTurnAllocation(turnAllocation->getKey()) == 0);   
   mTurnAllocationMap[turnAllocation->getKey()] = turnAllocation;
}

void 
TurnAllocationManager::removeTurnAllocation(const TurnAllocationKey& turnAllocationKey)
{
   TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
   if(it != mTurnAllocationMap.end())
   {
      delete it->second;
      mTurnAllocationMap.erase(it);
   }
}

TurnAllocation* 
TurnAllocationManager::findTurnAllocation(const TurnAllocationKey& turnAllocationKey)
{
   TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
   if(it != mTurnAllocationMap.end())
   {
      return it->second;
   }
   return 0;
}

TurnAllocation* 
TurnAllocationManager::findTurnAllocation(const StunTuple& requestedTuple)
{
   TurnAllocationMap::iterator it;
   for(it = mTurnAllocationMap.begin(); it != mTurnAllocationMap.end(); it++)
   {
      if(it->second->getRequestedTuple() == requestedTuple)
      {
         return it->second;
      }
   }
   return 0;
}

void 
TurnAllocationManager::allocationExpired(const asio::error_code& e, const TurnAllocationKey& turnAllocationKey)
{
   if (e != asio::error::operation_aborted)  // Note: nothing currently stops timers
   {
      // Timer was not cancelled, take necessary action.
      InfoLog(<< "Turn Allocation Expired! clientLocal=" << turnAllocationKey.getClientLocalTuple() << " clientRemote=" << turnAllocationKey.getClientRemoteTuple());

      TurnAllocationMap::iterator it = mTurnAllocationMap.find(turnAllocationKey);
      if(it != mTurnAllocationMap.end())
      {
         if(time(0) >= it->second->getExpires())
         {
            delete it->second;
            mTurnAllocationMap.erase(it);
         }
      }
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
