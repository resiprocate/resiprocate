#ifndef ProducerFifoBuffer_Include_Guard
#define ProducerFifoBuffer_Include_Guard

#include "rutil/Fifo.hxx"

namespace resip
{

/**
   Class for buffering messages placed in a Fifo, to be used by a single 
   producer. By adding messages in bulk, lock contention is reduced.
   @todo Refactor so we can use this with AbstractFifo<T>? This is presently not 
      the case because AbstractFifo does not expose its API publicly.
*/
template<typename T>
class ProducerFifoBuffer
{
   public:
      ProducerFifoBuffer(Fifo<T>& fifo,
                           size_t bufferSize) :
         mFifo(fifo),
         mBufferSize(bufferSize)
      {}

      ~ProducerFifoBuffer()
      {
         flush();
      }

      void add(T* msg)
      {
         mBuffer.push_back(msg);
         if(mBuffer.size()>=mBufferSize)
         {
            flush();
         }
      }

      void flush()
      {
         if(mBuffer.size() > 0)
         {
            mFifo.addMultiple(mBuffer);
         }
      }

      inline size_t getBufferSize() const
      {return mBufferSize;}

      inline void setBufferSize(size_t pBufferSize)
      {
         mBufferSize = pBufferSize;
         if(mBuffer.size()>=mBufferSize)
         {
            flush();
         }
      }

      inline const Fifo<T>& getFifo() const {return mFifo;} 
      inline Fifo<T>& getFifo() {return mFifo;} 

   protected:
      Fifo<T>& mFifo;
      // The type used by AbstractFifo<T>::addMultiple()
      typename Fifo<T>::Messages mBuffer;
      size_t mBufferSize;
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
