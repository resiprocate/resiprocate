#ifndef RESIP_TransactionMessage_hxx
#define RESIP_TransactionMessage_hxx

#include <cassert>
#include "resip/stack/Message.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class TransactionMessage : public Message
{
   public:
      RESIP_HeapCount(TransactionMessage);

      TransactionMessage& operator=(const TransactionMessage& rhs);

      virtual const Data& getTransactionId() const=0; 

      // indicates this message is associated with a Client Transaction for the
      // purpose of determining which TransactionMap to use
      virtual bool isClientTransaction() const;

      virtual Message* clone() const {assert(false); return NULL;}

      virtual void addBuffer(char* buf) { assert(false); } ;
      virtual void addBuffer(char* buf, int size) { assert(false); } ;

      void setFromTU() 
      {
         mIsExternal = false;
      }

      void setFromExternal()
      {
         mIsExternal = true;
      }
      
      bool isExternal() const
      {
         return mIsExternal;
      }

      bool isRequest() const;
      bool isResponse() const;

      // Returns the source tuple that the message was received from
      // only makes sense for messages received from the wire
      void setSource(const Tuple& tuple) { mSource = tuple; }
      const Tuple& getSource() const { return mSource; }
      
      // Used by the stateless interface to specify where to send a request/response
      void setDestination(const Tuple& tuple) { mDestination = tuple; }
      Tuple& getDestination() { return mDestination; }

   protected:
      TransactionMessage(const Transport* fromWire); 
      TransactionMessage(); 
      
      // !jf!
      const Transport* mTransport;

      UInt64 mCreatedTime;

      // For messages received from the wire, this indicates where it came
      // from. Can be used to get to the Transport and/or reliable Connection
      Tuple mSource;
      // Used by the TU to specify where a message is to go
      Tuple mDestination;

      // indicates this message came from the wire, set by the Transport
      bool mIsExternal;
      
      // is a request or response
      mutable bool mRequest;
      mutable bool mResponse;
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2004 Vovida Networks, Inc.  All rights reserved.
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

