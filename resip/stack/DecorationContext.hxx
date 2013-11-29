#if !defined(RESIP_DECORATION_CONTEXT_HXX)
#define RESIP_DECORATION_CONTEXT_HXX

#include "rutil/Data.hxx"
#include "resip/stack/MessageDecorator.hxx"
#include "resip/stack/SendData.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/TransportSelector.hxx"

namespace resip
{

class DecorationContext : public TransactionMessage
{
   public:
      DecorationContext(const resip::Data& tid, bool isClient,
            SipMessage &msg,
                        Transport* transport,
                        const Tuple &source,
                        const Tuple &destination,
                        const Data& sigCompId,
                        std::vector<MessageDecorator*> decorators,
                        bool& mDone,
                        TransportSelector& transportSelector,
                        SendData* sendData);

      virtual ~DecorationContext(){};

      virtual const Data& getTransactionId() const {return mTid;}

      TransportSelector& getTransportSelector() { return mTransportSelector; };

      /* @returns true when all done, false if async in progress
       */
      bool callOutboundDecorators();

      /* As asynchronous decorator must call this method when it
       * completes processing.
       */
      void decoratorFinished();

      /* Do the final encoding and send the message to the transport
       */
      void encodeAndSend();

      // indicates this message is associated with a Client Transaction for the
      // purpose of determining which TransactionMap to use
      virtual bool isClientTransaction() const {return mIsClientTransaction;}

      /// output the entire message to stream
      virtual EncodeStream& encode(EncodeStream& strm) const
      {
         strm << (mIsClientTransaction ? Data("Client ") : Data("Server ") )
               << Data("DnsResultMessage: tid=") << mTid;
         return strm;
      }
      /// output a brief description to stream
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const
      {
         strm << (mIsClientTransaction ? Data("Client ") : Data("Server ") )
               << Data("DnsResultMessage: tid=") << mTid;
         return strm;
      }

   private:

      resip::Data mTid;
      bool mIsClientTransaction;
      SipMessage &mMessage;
      Transport* mTransport;
      Tuple mSource;
      Tuple mDestination;
      Data mSigCompId;
      std::vector<MessageDecorator*> mDecorators;
      bool& mDone;
      TransportSelector& mTransportSelector;
      SendData* mSendData;

      std::vector<MessageDecorator*>::iterator mIterator;

};

EncodeStream&
operator<<(EncodeStream& strm, const DecorationContext& msg);

}
#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */


