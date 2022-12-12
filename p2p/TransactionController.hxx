#if !defined(P2P_TRANSACTION_CONTROLLER_HXX)
#define P2P_TRANSACTION_CONTROLLER_HXX

#include <map>
#include "rutil/Fifo.hxx"

namespace p2p
{

class TransactionState;

class TransactionMessage
{
public:
private:
};

class TimerMessage : public TransactionMessage
{
public:
private:
};

class TransactionController
{
   public:
      TransactionController();
      ~TransactionController();

   private:
      TransactionController(const TransactionController& rhs);
      TransactionController& operator=(const TransactionController& rhs);
      
      /// Get a message into the transaction controller
      void send(TransactionMessage* msg);
      void process();

      resip::Fifo<TransactionMessage> mStateMacFifo;

      // stores all of the transactions that are currently active in this stack 
      typedef std::map<uint64_t, TransactionState*> TransactionMap;
      TransactionMap mClientTransactionMap;  // used to send retransmission
      TransactionMap mServerTransactionMap;  // used to absorb retransmissions

      // timers associated with the transactions. When a timer fires, it is
      // placed in the mStateMacFifo
      //TimerQueue  mTimers;

      friend class TransactionState;
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

