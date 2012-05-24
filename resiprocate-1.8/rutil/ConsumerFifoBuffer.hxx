#ifndef ConsumerFifoBuffer_Include_Guard
#define ConsumerFifoBuffer_Include_Guard

#include "rutil/Fifo.hxx"

namespace resip
{
template<typename T>
class ConsumerFifoBuffer
{
   public:
      ConsumerFifoBuffer(Fifo<T>& fifo,
                           unsigned int bufferSize=8) :
         mFifo(fifo),
         mBufferSize(bufferSize)
      {}

      ~ConsumerFifoBuffer()
      {}

      T* getNext(int ms=0)
      {
         if(mBuffer.empty())
         {
            mFifo.getMultiple(ms, mBuffer, mBufferSize);
         }

         if(mBuffer.empty())
         {
            return 0;
         }

         T* next(mBuffer.front());
         mBuffer.pop_front();
         return next;
      }

      bool messageAvailable() const
      {
         if(mBuffer.empty())
         {
            return mFifo.messageAvailable();
         }
         return true;
      }

      unsigned int size() const
      {
         return mBuffer.size() + mFifo.size();
      }

   private:
      Fifo<T>& mFifo;
      typename Fifo<T>::Messages mBuffer;
      unsigned int mBufferSize;
};
}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
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
