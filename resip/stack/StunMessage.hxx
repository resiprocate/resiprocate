#if !defined(RESIP_STUNMESSAGE_HXX)
#define RESIP_STUNMESSAGE_HXX 

//#include <sys/types.h>
//#include <list>
//#include <vector>
//#include <utility>
//#include <memory> 

#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/compat.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/stun/Stun.hxx"

namespace resip
{

class Contents;
class ExtensionHeader;
class SecurityAttributes;
class Transport;

class StunMessage : public TransactionMessage
{
   public:
       explicit StunMessage(const Transport* fromWire = 0);
      // .dlb. public, allows pass by value to compile.
      StunMessage(const StunMessage& message);

      // .dlb. sure would be nice to have overloaded return value here..
      virtual Message* clone() const;

      StunMessage& operator=(const StunMessage& rhs);
      
      // returns the transaction id from the branch or if 2543, the computed hash
      virtual const Data& getTransactionId() const;

      virtual ~StunMessage();

      static StunMessage* make(const Data& buffer, bool isExternal = false);

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "StunMessage::Exception"; }
      };

      virtual bool isClientTransaction() const;
      
      virtual std::ostream& encode(std::ostream& str) const;      
      
      virtual std::ostream& encodeBrief(std::ostream& str) const;
            
      virtual void addBuffer(char* buf);
      
      // returns the encoded buffer which was encoded by resolve()
      // should only be called by the TransportSelector
      Data& getEncoded();
      
      UInt64 getCreatedTimeMicroSec() {return mCreatedTime;}
      
   protected:
      void cleanUp();
      
   private:
      
      // Raw buffers coming from the Transport. message manages the memory
      std::vector<char*> mBufferList;

      StunMessageStruct  mStunMessageStruct;

//      Data mEncoded; // to be retransmitted
      
      friend class TransportSelector;
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
