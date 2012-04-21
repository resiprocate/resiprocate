#ifndef StlPoolAllocator_Include_Guard
#define StlPoolAllocator_Include_Guard

#include <limits>
#include <memory>
#include <stddef.h>

// .bwc. gcc 4.2 and above support stateful allocators. I don't know about other 
// compilers; if you do, add them here please.
#if ( (__GNUC__ == 4) && (__GNUC_MINOR__ >= 2) ) || ( __GNUC__ > 4 )
#define RESIP_HAS_STATEFUL_ALLOCATOR_SUPPORT
#endif

#ifdef max  // Max is defined under WIN32 and conflicts with std::numeric_limits<size_type>::max use below
#undef max
#endif

namespace resip
{
/**
   A dirt-simple lightweight stl pool allocator meant for use in short-lifetime 
   objects. This will pool-allocate at most S bytes, after which no further pool
   allocation will be performed, and fallback to the system new/delete will be 
   used (deallocating a pool allocated object will _not_ free up room in the 
   pool).
*/
template<typename T, typename P>
class StlPoolAllocator
{
   public:
      typedef T value_type;
      typedef value_type* pointer;
      typedef const value_type* const_pointer;
      typedef value_type& reference;
      typedef const value_type& const_reference;
      typedef size_t size_type;
      typedef ptrdiff_t difference_type;

#ifdef RESIP_HAS_STATEFUL_ALLOCATOR_SUPPORT
      explicit StlPoolAllocator(P* pool=0) :
         mPool(pool){}
      StlPoolAllocator(const StlPoolAllocator& other) :
         mPool(other.mPool){}
      StlPoolAllocator& operator=(const StlPoolAllocator& rhs)
      {
         mPool=rhs.mPool;
         return *this;
      }
#else
      // Disable pool allocation if stateful allocators are not supported by the 
      // STL
      explicit StlPoolAllocator(P* pool=0) :
         mPool(0){}
      StlPoolAllocator(const StlPoolAllocator& other) :
         mPool(0){}
      StlPoolAllocator& operator=(const StlPoolAllocator& rhs)
      {
         return *this;
      }
#endif


      template<typename U>
      StlPoolAllocator(const StlPoolAllocator<U,P>& other) :
         mPool(other.mPool){}

      ~StlPoolAllocator(){}


      template<typename U>
      struct rebind
      {
         typedef StlPoolAllocator<U, P> other;
      };

      pointer address(reference ref) const
      {
         return &ref;
      }

      const_pointer address(const_reference ref) const
      {
         return &ref;
      }

      pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0)
      {
         return (pointer)(allocate_raw(n*sizeof(T)));
      }

      void deallocate(pointer ptr, size_type n)
      {
         deallocate_raw((void*)ptr);
      }

      inline void* allocate_raw(size_type bytes)
      {
         if(mPool)
         {
            return mPool->allocate(bytes);
         }
         return ::operator new(bytes);
      }

      void deallocate_raw(void* ptr)
      {
         if(mPool)
         {
            mPool->deallocate(ptr);
            return;
         }
         ::operator delete(ptr);
      }

      size_type max_size() const
      {
         if(mPool)
         {
            return mPool->max_size();
         }
         return std::numeric_limits<size_type>::max()/sizeof(T);
      }

      size_type max_size(size_type _sz) const
      {
         if(mPool)
         {
            return mPool->max_size();
         }
         return std::numeric_limits<size_type>::max()/_sz;
      }

      void construct(pointer p, const_reference orig)
      {
         new (p) T(orig);
      }

      void destroy(pointer ptr)
      {
         ptr->~T();
      }

      bool operator==(const StlPoolAllocator& rhs) const
      {
         return mPool == rhs.mPool;
      }

      bool operator!=(const StlPoolAllocator& rhs) const
      {
         return mPool != rhs.mPool;
      }

      P* mPool;
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
