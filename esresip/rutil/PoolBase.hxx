#ifndef PoolBase_Include_Guard
#define PoolBase_Include_Guard

#include <stddef.h>

namespace resip
{
class PoolBase
{
   public:
      virtual ~PoolBase(){}
      virtual void* allocate(size_t size)=0;
      virtual void deallocate(void* ptr)=0;
      virtual size_t max_size() const=0;
};

}

void* operator new(size_t size, resip::PoolBase* pool);
void operator delete(void* ptr, resip::PoolBase* pool);

#endif
