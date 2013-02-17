#ifndef __OSC__MUTEX_LOCKABLE
#define __OSC__MUTEX_LOCKABLE 1

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
  @file MutexLockable.h
  @brief Header file for osc::MutexLockable class.
*/


#ifdef USE_POSIX_LOCKING
#include <pthread.h>
#endif

namespace osc
{
  /**
    Provides mutex functionality for multithread synchronization.

    This class is not currently used by the Open SigComp library.

    @see ReadWriteLockable
  */

  class MutexLockable
  {
    public:
      MutexLockable();
      ~MutexLockable();
 
      MutexLockable * operator &(){ return this; }
      MutexLockable const * operator &() const { return this; }

      bool lock();
      bool unlock();

    protected:

    private:
      /* if you define these, move them to public */
      MutexLockable(MutexLockable const &);
      MutexLockable& operator=(MutexLockable const &);

#ifdef USE_POSIX_LOCKING
      pthread_mutex_t _mutex;
#endif
  };
}

#ifdef USE_POSIX_LOCKING
inline osc::MutexLockable::MutexLockable()
{
  pthread_mutex_init(&_mutex,0);
}

inline osc::MutexLockable::~MutexLockable()
{
  pthread_mutex_destroy(&_mutex);
}

inline bool osc::MutexLockable::lock()
{
  return pthread_mutex_lock(&_mutex) ? false : true;
}

inline bool osc::MutexLockable::unlock()
{
  return pthread_mutex_unlock(&_mutex) ? false : true;
}
#else
#warning No locking enabled: not mulithread safe
inline osc::MutexLockable::MutexLockable() { } 
inline osc::MutexLockable::~MutexLockable() { } 
inline bool osc::MutexLockable::lock() { return true; } 
inline bool osc::MutexLockable::unlock() { return true; }
#endif

#endif
