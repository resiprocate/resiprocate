#ifndef THREADS_H_
#define THREADS_H_

////////////////////////////////////////////////////////////////////////////////
// macro DEFAULT_THREADING
// Selects the default threading model for certain components of Loki
// If you don't define it, it defaults to single-threaded
// All classes in Loki have configurable threading model; DEFAULT_THREADING
// affects only default template arguments
////////////////////////////////////////////////////////////////////////////////

// Last update: June 20, 2001

#ifndef DEFAULT_THREADING
#define DEFAULT_THREADING /**/ ::Loki::SingleThreaded
#endif

#ifdef __linux__
//#include <asm/atomic.h>
#include <pthread.h>
#endif

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template SingleThreaded
// Implementation of the ThreadingModel policy used by various classes
// Implements a single-threaded model; no synchronization
////////////////////////////////////////////////////////////////////////////////

template <class Host>
class SingleThreaded
{
   public:
      struct Lock
      {
            Lock() {}
            Lock(const Host&) {}
      };
        
      typedef Host VolatileType;

      typedef int IntType; 

      static IntType AtomicAdd(volatile IntType& lval, IntType val)
      { return lval += val; }
        
      static IntType AtomicSubtract(volatile IntType& lval, IntType val)
      { return lval -= val; }

      static IntType AtomicMultiply(volatile IntType& lval, IntType val)
      { return lval *= val; }
        
      static IntType AtomicDivide(volatile IntType& lval, IntType val)
      { return lval /= val; }
        
      static IntType AtomicIncrement(volatile IntType& lval)
      { return ++lval; }
        
      static IntType AtomicDivide(volatile IntType& lval)
      { return --lval; }
        
      static void AtomicAssign(volatile IntType & lval, IntType val)
      { lval = val; }
        
      static void AtomicAssign(IntType & lval, volatile IntType & val)
      { lval = val; }
};

#ifndef WIN32
template <class Host>
class ClassLevelLockable
{
      static pthread_mutex_t mtx_;

      struct Initializer;
      friend struct Initializer;
      struct Initializer
      {
            Initializer()
            {
               pthread_mutex_init(&mtx_, NULL);
            }
            ~Initializer()
            {
               pthread_mutex_destroy(&mtx_);
            }
      };
          
      static Initializer initializer_;

   public:
      class Lock;
      friend class Lock;
        
      class Lock
      {
            Lock(const Lock&);
            Lock& operator=(const Lock&);
         public:
            Lock()
            {
               pthread_mutex_lock(&mtx_);
            }
            Lock(Host&)
            {
               pthread_mutex_lock(&mtx_);
            }
            ~Lock()
            {
               pthread_mutex_unlock(&mtx_);
            }
      };

      typedef volatile Host VolatileType;

      typedef long IntType; 

      static IntType AtomicIncrement(volatile IntType& lval)
      { Lock l; return lval++; }
      
      static IntType AtomicDecrement(volatile IntType& lval)
      { Lock l; return --lval; }
        
      //static void AtomicAssign(volatile IntType& lval, IntType val)
      //{ atomic_set(static_cast<atomic_t&>(lval), val); }
      
      //static void AtomicAssign(IntType& lval, volatile IntType& val)
      //{ atomic_set(static_cast<atomic_t&>(lval), val); };
};

template <class Host>
class ObjectLevelLockable
{
      pthread_mutex_t mtx_;

   public:
      ObjectLevelLockable()
      {
         pthread_mutex_init(&mtx_, NULL);
      }

      ~ObjectLevelLockable()
      {
         pthread_mutex_destroy(&mtx_);
      }

      class Lock;
      friend class Lock;
        
      class Lock
      {
            ObjectLevelLockable& host_;
            
            Lock(const Lock&);
            Lock& operator=(const Lock&);
         public:
            Lock(Host& host) : host_(host)
            {
               pthread_mutex_lock(&host_.mtx_);
            }
            ~Lock()
            {
               pthread_mutex_unlock(&host_.mtx_);
            }
      };

      typedef volatile Host VolatileType;

      typedef long IntType; 

#if 0
      static IntType AtomicIncrement(volatile IntType& lval)
      { return atomic_inc_and_test(static_cast<atomic_t&>(lval)); }
          
      static IntType AtomicDecrement(volatile IntType& lval)
      { return atomic_dec_and_test(static_cast<atomic_t&>(lval)); }
        
      static void AtomicAssign(volatile IntType& lval, IntType val)
      { atomic_set(static_cast<atomic_t&>(lval), val); }
      
      static void AtomicAssign(IntType& lval, volatile IntType& val)
      { atomic_set(static_cast<atomic_t&>(lval), val); };
#endif
};

template <class Host>
pthread_mutex_t ClassLevelLockable<Host>::mtx_;

template <class Host>
typename ClassLevelLockable<Host>::Initializer 
ClassLevelLockable<Host>::initializer_;
          
#endif
    
#ifdef _WINDOWS_

////////////////////////////////////////////////////////////////////////////////
// class template ObjectLevelLockable
// Implementation of the ThreadingModel policy used by various classes
// Implements a object-level locking scheme
////////////////////////////////////////////////////////////////////////////////

template <class Host>
class ObjectLevelLockable
{
      CRITICAL_SECTION mtx_;

   public:
      ObjectLevelLockable()
      {
         ::InitializeCriticalSection(&mtx_);
      }

      ~ObjectLevelLockable()
      {
         ::DeleteCriticalSection(&mtx_);
      }

      class Lock;
      friend class Lock;
        
      class Lock
      {
            ObjectLevelLockable& host_;
            
            Lock(const Lock&);
            Lock& operator=(const Lock&);
         public:
            Lock(Host& host) : host_(host)
            {
               ::EnterCriticalSection(&host_.mtx_);
            }
            ~Lock()
            {
               ::LeaveCriticalSection(&host_.mtx_);
            }
      };

      typedef volatile Host VolatileType;

      typedef LONG IntType; 

      static IntType AtomicIncrement(volatile IntType& lval)
      { return InterlockedIncrement(&const_cast<IntType&>(lval)); }
        
      static IntType AtomicDivide(volatile IntType& lval)
      { return InterlockedDecrement(&const_cast<IntType&>(lval)); }
        
      static void AtomicAssign(volatile IntType& lval, IntType val)
      { InterlockedExchange(&const_cast<IntType&>(lval), val); }
        
      static void AtomicAssign(IntType& lval, volatile IntType& val)
      { InterlockedExchange(&lval, val); }
};
    
template <class Host>
class ClassLevelLockable
{
      static CRITICAL_SECTION mtx_;

      struct Initializer;
      friend struct Initializer;
      struct Initializer
      {
            Initializer()
            {
               ::InitializeCriticalSection(&mtx_);
            }
            ~Initializer()
            {
               ::DeleteCriticalSection(&mtx_);
            }
      };
        
      static Initializer initializer_;

   public:
      class Lock;
      friend class Lock;
        
      class Lock
      {
            Lock(const Lock&);
            Lock& operator=(const Lock&);
         public:
            Lock()
            {
               ::EnterCriticalSection(&mtx_);
            }
            Lock(Host&)
            {
               ::EnterCriticalSection(&mtx_);
            }
            ~Lock()
            {
               ::LeaveCriticalSection(&mtx_);
            }
      };

      typedef volatile Host VolatileType;

      typedef LONG IntType; 

      static IntType AtomicIncrement(volatile IntType& lval)
      { return InterlockedIncrement(&const_cast<IntType&>(lval)); }
        
      static IntType AtomicDivide(volatile IntType& lval)
      { return InterlockedDecrement(&const_cast<IntType&>(lval)); }
        
      static void AtomicAssign(volatile IntType& lval, IntType val)
      { InterlockedExchange(&const_cast<IntType&>(lval), val); }
        
      static void AtomicAssign(IntType& lval, volatile IntType& val)
      { InterlockedExchange(&lval, val); }
};
    
template <class Host>
CRITICAL_SECTION ClassLevelLockable<Host>::mtx_;
    
template <class Host>
typename ClassLevelLockable<Host>::Initializer 
ClassLevelLockable<Host>::initializer_;
    
#endif    
}

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
////////////////////////////////////////////////////////////////////////////////

#endif
