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

#if defined(__linux__) || defined(__CYGWIN32__)
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
};

#if defined(__linux__) || defined(__CYGWIN32__)
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
               pthread_mutex_destroy(&mtx);
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
};

template <class Host>
pthread_mutex_t ClassLevelLockable<Host>::mtx_;

template <class Host>
typename ClassLevelLockable<Host>::Initializer 
ClassLevelLockable<Host>::initializer_;
          
#endif
    
#ifdef _WINDOWS_ 

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
