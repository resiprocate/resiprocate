#if !defined(RESIP_TuSelector_HXX)
#define RESIP_TuSelector_HXX 

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/StatisticsMessage.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Mutex.hxx"

namespace resip
{

class ConnectionTerminated;
class KeepAlivePong;
class Message;
class TransactionUser;
class AsyncProcessHandler;

// Members that read or change the TU list are for the thread that runs the
// TransactionController, except addFromAnyThread(),
// addPendingTransactionUser() and setCongestionManager(), which any thread
// may call, and registerTransactionUser(), which is only for use before
// processing starts.
class TuSelector
{
   public:
      TuSelector(TimeLimitFifo<Message>& fallBackFifo);
      ~TuSelector();
      
      void add(Message* msg, TimeLimitFifo<Message>::DepthUsage usage);
      void addFromAnyThread(Message* msg, TimeLimitFifo<Message>::DepthUsage usage);
      void add(ConnectionTerminated* term);
      void add(KeepAlivePong* pong);
      
      unsigned int size() const;      
      bool wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const;
  
      TransactionUser* selectTransactionUser(const SipMessage& msg);
      bool haveTransactionUsers() const { return mTuSelectorMode; }
      void registerTransactionUser(TransactionUser&, const bool front = false);
      // Messages from addFromAnyThread() reach the TU from this call on. The
      // caller must queue a RegisterTransactionUser(AtFront) message for the
      // TU, which process() applies.
      void addPendingTransactionUser(TransactionUser&);
      void process(TransactionUserMessage& msg);
      bool isTransactionUserStillRegistered(const TransactionUser* ) const;

      // Handler is notified when a message is posted
      // to the default application receive queue.
      void setFallbackPostNotify(AsyncProcessHandler *handler);
      
      void setCongestionManager(CongestionManager* manager);
      CongestionManager::RejectionBehavior getRejectionBehavior(TransactionUser* tu) const;
      uint32_t getExpectedWait(TransactionUser* tu) const;

   private:
      void activatePendingTransactionUser(TransactionUser& tu, const bool front);
      void insert(TransactionUser& tu, const bool front);
      void remove(TransactionUser* tu);
      void markShuttingDown(TransactionUser* tu);
      bool exists(TransactionUser* tu) const;

   private:
      struct Item
      {
            Item(TransactionUser* ptu) : tu(ptu), shuttingDown(false) {}
            TransactionUser* tu;
            bool shuttingDown;
            bool operator==(const Item& rhs) { return tu == rhs.tu; }
      };
      
      typedef std::vector<Item> TuList;
      TuList mTuList;
      // TUs whose registration is queued but not yet applied.
      typedef std::vector<TransactionUser*> PendingTuList;
      PendingTuList mPendingTus;
      // Held by other threads while they read the TU list, and by the owner
      // thread only while it adds or removes a TU.
      Mutex mTuListMutex;
      TimeLimitFifo<Message>& mFallBackFifo;
      CongestionManager* mCongestionManager;
      AsyncProcessHandler *mFallbackPostNotify;
      bool mTuSelectorMode;
      StatisticsMessage::Payload mStatsPayload;
};
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
