#ifndef __OSC__READ_WRITE_LOCKABLE
#define __OSC__READ_WRITE_LOCKABLE 1

/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file ReadWriteLockable.h
  @brief Header file for osc::ReadWriteLockable class.
*/


#ifdef USE_POSIX_LOCKING
#include <pthread.h>
#endif

#ifdef USE_WINDOWS_LOCKING
#include "Windows.h"
#endif

namespace osc
{
  /**
    Provides Read/Write Lock functionality for multithread synchronization.

    If the Open SigComp library is compiled for multithreaded use,
    this class must have an implementation of read/write locking
    suitable for the environment in which it is being used.
    The current implementation uses pthread rwlock structures;
    however, adding new read/write lock primatives should be
    trivial.

    Classes that might be accessed from multiple threads
    simultaneously are derived from this class. All users are
    required to lock such classes appropriately before using them.
  */

  class ReadWriteLockable
  {
    public:
      ReadWriteLockable();
      ~ReadWriteLockable();
 
      ReadWriteLockable * operator &(){ return this; }
      ReadWriteLockable const * operator &() const { return this; }

      bool readLock();
      bool writeLock();
      bool unlock();

    protected:

    private:
      /* if you define these, move them to public */
      ReadWriteLockable(ReadWriteLockable const &);
      ReadWriteLockable& operator=(ReadWriteLockable const &);

#ifdef USE_POSIX_LOCKING
      pthread_rwlock_t _rwlock;
#elif defined(USE_WINDOWS_LOCKING)
      CRITICAL_SECTION mCriticalSection;
#endif

  };
}

#ifdef USE_POSIX_LOCKING
inline osc::ReadWriteLockable::ReadWriteLockable()
{
  pthread_rwlock_init(&_rwlock,0);
}

inline osc::ReadWriteLockable::~ReadWriteLockable()
{
  pthread_rwlock_destroy(&_rwlock);
}

inline bool osc::ReadWriteLockable::readLock()
{
  return pthread_rwlock_rdlock(&_rwlock) ? false : true;
}

inline bool osc::ReadWriteLockable::writeLock()
{
  return pthread_rwlock_wrlock(&_rwlock) ? false : true;
}

inline bool osc::ReadWriteLockable::unlock()
{
  return pthread_rwlock_unlock(&_rwlock) ? false : true;
}
#elif defined (USE_WINDOWS_LOCKING)
inline osc::ReadWriteLockable::ReadWriteLockable()
{
  InitializeCriticalSection(&mCriticalSection);
}

inline osc::ReadWriteLockable::~ReadWriteLockable()
{
  DeleteCriticalSection(&mCriticalSection);
}

inline bool osc::ReadWriteLockable::readLock()
{
  EnterCriticalSection(&mCriticalSection);
  return true;
}

inline bool osc::ReadWriteLockable::writeLock()
{
  EnterCriticalSection(&mCriticalSection);
  return true;
}

inline bool osc::ReadWriteLockable::unlock()
{
  LeaveCriticalSection(&mCriticalSection);
  return true;
}
#else
#ifdef _MSC_VER
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)"): "
#pragma message (__LOC__ " No locking enabled: not mulithread safe ") 
#else
#warning No locking enabled: not mulithread safe
#endif
inline osc::ReadWriteLockable::ReadWriteLockable() { }
inline osc::ReadWriteLockable::~ReadWriteLockable() { }
inline bool osc::ReadWriteLockable::readLock() { return true; }
inline bool osc::ReadWriteLockable::writeLock() { return true; }
inline bool osc::ReadWriteLockable::unlock() { return true; }
#endif

#endif
