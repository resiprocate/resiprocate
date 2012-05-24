#ifndef PROCESSOR_MESSAGE_HXX
#define PROCESSOR_MESSAGE_HXX 1

#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "repro/Processor.hxx"

namespace repro
{

class ProcessorMessage : public resip::ApplicationMessage
{
   public:
   
      ProcessorMessage(const Processor& proc,
                       const resip::Data& tid,
                       resip::TransactionUser* tupassed):
         mTid(tid)
      {
         mTu = tupassed;
         mReturnAddress = proc.getAddress();
         mOriginatorAddress = mReturnAddress;
         mType = proc.getChainType();
      }

      ProcessorMessage(const ProcessorMessage& orig) :
         resip::ApplicationMessage(orig),
         mTid(orig.mTid)
      {
         mReturnAddress=orig.mReturnAddress;
         mOriginatorAddress=orig.mOriginatorAddress;
         mType=orig.mType;
      }

      virtual ~ProcessorMessage(){}

      void pushAddr(int addr)
      {
         mReturnAddress.push_back(addr);
      }
      
      int popAddr()
      {
         if(mReturnAddress.empty())
         {
            return 0;
         }
         
         int addr = mReturnAddress.back();
         mReturnAddress.pop_back();
         return addr;
      }
      
      std::vector<short>& getOriginatorAddress()
      {
         return mOriginatorAddress;
      }

      Processor::ChainType chainType() const
      {
         return mType;
      }

      virtual Message* clone() const = 0;

      virtual EncodeStream& encode(EncodeStream& strm) const = 0;
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const = 0;

      virtual const resip::Data& getTransactionId() const
      {
         return mTid;
      }

   protected:
      resip::Data mTid;
      std::vector<short> mReturnAddress;
      std::vector<short> mOriginatorAddress;
      Processor::ChainType mType;
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
