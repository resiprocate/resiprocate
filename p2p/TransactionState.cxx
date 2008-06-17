#include <assert.h>
#include <iostream>
#include "rutil/Logger.hxx"

#include "p2p/TransactionState.hxx"
#include "p2p/TransactionController.hxx"
#include "p2p/Message.hxx"
#include "p2p/P2PSubsystem.hxx"

using namespace p2p;

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

TransactionState::TransactionState(TransactionController& controller, Machine m, 
                                   State s, UInt64 tid) : 
   mController(controller),
   mMachine(m), 
   mState(s),
   mMsgToRetransmit(0),
   mTid(tid)
{
   add(tid);
   StackLog (<< "Creating new TransactionState: " << *this);
}


TransactionState::~TransactionState()
{
   //StackLog (<< "Deleting TransactionState " << mId << " : " << this);
   erase(mTid);
   
   delete mMsgToRetransmit;
   mMsgToRetransmit = 0;
}


void
TransactionState::add(UInt64 tid)
{
   if (isClient())
   {
      mController.mClientTransactionMap[tid] = this;
   }
   else
   {
      mController.mServerTransactionMap[tid] = this;
   }
}

void
TransactionState::erase(UInt64 tid)
{
   if (isClient())
   {
      mController.mClientTransactionMap.erase(tid);
   }
   else
   {
      mController.mServerTransactionMap.erase(tid);
   }
}


bool 
TransactionState::isClient() const
{
   switch(mMachine)
   {
      case ClientRequest:
         return true;
      case ServerRequest:
         return false;
      default:
         assert(0);
   }
   return false;
}

std::ostream& 
p2p::operator<<(std::ostream& strm, const p2p::TransactionState& state)
{
   strm << "tid=" << state.mTid << " [ ";
   switch (state.mMachine)
   {
      case TransactionState::ClientRequest:
         strm << "ClientRequest";
         break;
      case TransactionState::ServerRequest:
         strm << "ServerRequest";
         break;
      default:
         strm << "Unknown(" << state.mMachine << ")";
   }
   
   strm << "/";
   switch (state.mState)
   {
      case TransactionState::Initial:
         strm << "Initial";
         break;
      case TransactionState::Trying:
         strm << "Trying";
         break;
      case TransactionState::Completed:
         strm << "Completed";
         break;
      default:
         strm << "Unknown(" << state.mState << ")";
   }
   
   strm << "]";
   return strm;
}


/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
