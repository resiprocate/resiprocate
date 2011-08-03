#include "rutil/PoolBase.hxx"

void* operator new(size_t size, resip::PoolBase* pool)
{
   if(pool)
   {
      return pool->allocate(size);
   }
   return ::operator new(size);
}

void operator delete(void* ptr, resip::PoolBase* pool)
{
   if(pool)
   {
      pool->deallocate(ptr);
      return;
   }
   ::operator delete(ptr);
}
