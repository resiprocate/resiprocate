#ifndef DinkyPool_Include_Guard
#define DinkyPool_Include_Guard

#include <limits>
#include <memory>
#include <stddef.h>

#include "rutil/PoolBase.hxx"

namespace resip
{
/**
   A dirt-simple lightweight pool allocator meant for use in short-lifetime 
   objects. This will pool-allocate at most S bytes, after which no further pool
   allocation will be performed, and fallback to the system new/delete will be 
   used (deallocating a pool allocated object will _not_ free up room in the 
   pool; the memory will be freed when the DinkyPool goes away).
*/
template<unsigned int S>
class DinkyPool : public PoolBase
{
   public:
      DinkyPool() : count(0), heapBytes(0) {}
      ~DinkyPool(){}

      void* allocate(size_t size)
      {
         if((8*count)+size <= S)
         {
            void* result=mBuf[count];
            count+=(size+7)/8;
            return result;
         }
         heapBytes += size;
         return ::operator new(size);
      }

      void deallocate(void* ptr)
      {
         if(ptr >= (void*)mBuf[0] && ptr < (void*)mBuf[(S+7)/8])
         {
            return;
         }
         ::operator delete(ptr);
      }

      size_t max_size() const
      {
         return std::numeric_limits<size_t>::max();
      }

      size_t getHeapBytes() const { return heapBytes; }
      size_t getPoolBytes() const { return count*8; }
      size_t getPoolSizeBytes() const { return sizeof(mBuf); }

   private:
      // disabled
      DinkyPool& operator=(const DinkyPool& rhs);
      DinkyPool(const DinkyPool& other);

      size_t count; // 8-byte chunks alloced so far
      char mBuf[(S+7)/8][8]; // 8-byte chunks for alignment
      size_t heapBytes;
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
