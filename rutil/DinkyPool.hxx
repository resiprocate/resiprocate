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
      DinkyPool() :
         count(0){}

      ~DinkyPool(){}

      void* allocate(size_t size)
      {
         if((8*count)+size <= S)
         {
            void* result=mBuf[count];
            count+=(size+7)/8;
            return result;
         }
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
   private:
      // disabled
      DinkyPool& operator=(const DinkyPool& rhs);
      DinkyPool(const DinkyPool& other);

      size_t count; // 8-byte chunks alloced so far
      char mBuf[(S+7)/8][8]; // 8-byte chunks for alignment
};

}
#endif
