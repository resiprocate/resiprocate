#if !defined(P2P_TRANSACTIONSTATE_HXX)
#define P2P_TRANSACTIONSTATE_HXX

#include "rutil/Data.hxx"

namespace p2p
{

class Message;
class TransactionController;

class TransactionState
{
   public:
      ~TransactionState();
     
   private:
      typedef enum 
      {
         ClientRequest,
         ServerRequest
      } Machine;
      
      typedef enum 
      {
         Initial,
         Trying,
         Completed  
      } State;

      TransactionState(TransactionController& controller, 
                       Machine m, 
                       State s, 
                       UInt64 tid);
      
      void processClientRequest(Message* msg);
      void processServerRequest(Message* msg);
      
      bool isClient() const;
      void add(UInt64 tid);
      void erase(UInt64 tid);
      
   private:
      
      TransactionController& mController;
      
      Machine mMachine;
      State mState;

      Message* mMsgToRetransmit;

      UInt64 mTid;
      
      friend std::ostream& operator<<(std::ostream& strm, const TransactionState& state);
      friend class TransactionController;
};


}

#endif


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
