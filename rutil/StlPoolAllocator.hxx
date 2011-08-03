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
