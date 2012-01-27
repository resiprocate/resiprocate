/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TuSelector_HXX)
#define RESIP_TuSelector_HXX 

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/StatisticsMessage.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Fifo.hxx"

namespace resip
{

class ConnectionTerminated;
class Message;
class TransactionUser;

/**
   @internal
*/
class TuSelector
{
   public:
      TuSelector(Fifo<Message>& fallBackFifo);
      ~TuSelector();
      
      void add(Message* msg);
      void add(ConnectionTerminated* term);
      
      unsigned int size() const;      
  
      TransactionUser* selectTransactionUser(const SipMessage& msg);
      bool haveTransactionUsers() const { return mTuSelectorMode; }
      void registerTransactionUser(TransactionUser&);
      void requestTransactionUserShutdown(TransactionUser&);
      void unregisterTransactionUser(TransactionUser&);
      void process();
      unsigned int getTimeTillNextProcessMS();
      bool isTransactionUserStillRegistered(const TransactionUser* ) const;

      void setCongestionManager(CongestionManager* manager);
      
   private:
      void remove(TransactionUser* tu);
      void markShuttingDown(TransactionUser* tu);
      bool exists(TransactionUser* tu);

   private:
      /**
         @internal
      */
      struct Item
      {
            Item(TransactionUser* ptu) : tu(ptu), shuttingDown(false) {}
            TransactionUser* tu;
            bool shuttingDown;
            bool operator==(const Item& rhs) { return tu == rhs.tu; }
      };
      
      typedef std::vector<Item> TuList;
      TuList mTuList;
      Fifo<Message>& mFallBackFifo;
      Fifo<TransactionUserMessage> mShutdownFifo;
      bool mTuSelectorMode;
      StatisticsMessage::Payload mStatsPayload;
};
}

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
