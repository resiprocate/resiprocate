#ifndef InvokeAfterSocketCreationFunc_Include_Guard
#define InvokeAfterSocketCreationFunc_Include_Guard

#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/Transport.hxx"

namespace resip
{
class InvokeAfterSocketCreationFunc : public TransactionMessage
{
   public:
      explicit InvokeAfterSocketCreationFunc(TransportType type) : mTransportType(type)
      {}
      virtual ~InvokeAfterSocketCreationFunc(){}

      virtual const Data& getTransactionId() const {return Data::Empty;}
      virtual TransportType getTransportType() const { return mTransportType; }

      virtual bool isClientTransaction() const {return true;}
      virtual EncodeStream& encode(EncodeStream& strm) const
      {
         return strm << "InvokeAfterSocketCreationFunc: type=" << mTransportType;
      }
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const
      {
         return strm << "InvokeAfterSocketCreationFunc: type=" << mTransportType;
      }

      virtual Message* clone() const
      {
         resip_assert(false); return NULL;
      }

   protected:
      TransportType mTransportType;
}; // class InvokeAfterSocketCreationFunc

} // namespace resip

#endif // include guard

/* ====================================================================
 *
 * Copyright 2016 SIP Spectrum, Inc http://www.sipspectrum.com  
 * All rights reserved.
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

