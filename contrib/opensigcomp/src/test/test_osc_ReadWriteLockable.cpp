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


#include <iostream>
#ifdef USE_POSIX_LOCKING
#include <pthread.h>
#include <unistd.h>
#include "ReadWriteLockable.h"
#include "TestList.h"

class TestRWLock : public osc::ReadWriteLockable {};

static TestRWLock *rwlock;
static int value;

static void *increment(void *arg)
{
  rwlock->writeLock();
  value++;
  rwlock->unlock();
  return 0;
}

static void *check(void *arg)
{
  rwlock->readLock();
  int val = value;
  rwlock->unlock();
  return (void *)(val);
}

bool test_osc_ReadWriteLockable()
{
  rwlock = new TestRWLock();

  value = 0;

  int status;
  pthread_t t1, t2, t3;
  void *result;

  // Check read lock interaction with write lock.
  rwlock->readLock();
  status = pthread_create(&t1, 0, increment, 0);
  TEST_ASSERT_EQUAL(status, 0);
  sched_yield();
  usleep(20);
  TEST_ASSERT_EQUAL(value, 0);
  rwlock->unlock();
  sched_yield();
  usleep(20);
  TEST_ASSERT_EQUAL(value, 1);
  TEST_ASSERT_EQUAL(status, 0);
  status = pthread_join(t1, &result);
  TEST_ASSERT_EQUAL(((int)(result)),0);

  // Check precidence of read versus write lock
  rwlock->writeLock();
  status = pthread_create(&t1, 0, increment, 0);
  TEST_ASSERT_EQUAL(status, 0);
  status = pthread_create(&t2, 0, check, 0);
  TEST_ASSERT_EQUAL(status, 0);
  status = pthread_create(&t3, 0, check, 0);
  TEST_ASSERT_EQUAL(status, 0);
  sched_yield();
  usleep(20);
  TEST_ASSERT_EQUAL(value, 1);
  rwlock->unlock();
  sched_yield();
  usleep(20);
  status = pthread_join(t1, &result);
  TEST_ASSERT_EQUAL(((int)(result)),0);
  TEST_ASSERT_EQUAL(value, 2);
  status = pthread_join(t2, &result);
  TEST_ASSERT_EQUAL(((int)(result)),2);
  status = pthread_join(t3, &result);
  TEST_ASSERT_EQUAL(((int)(result)),2);

  delete rwlock;
  return true;
}

static bool ReadWriteLockableTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_ReadWriteLockable,
                                     "test_osc_ReadWriteLockable");
#endif

