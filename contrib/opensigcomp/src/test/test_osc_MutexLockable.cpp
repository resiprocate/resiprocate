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
#include "MutexLockable.h"
#include "TestList.h"

class TestMutex : public osc::MutexLockable { };
static TestMutex *mutex;
static int value;

static void *increment(void *arg)
{
  mutex->lock();
  value++;
  mutex->unlock();
  return 0;
}

bool test_osc_MutexLockable()
{
  mutex = new TestMutex();
  value = 0;

  mutex->lock();

  int status;
  pthread_t thread;
  status = pthread_create(&thread, 0, increment, 0);
  TEST_ASSERT_EQUAL(status, 0);
  TEST_ASSERT_EQUAL(value, 0);

  mutex->unlock();

  usleep(20);
  TEST_ASSERT_EQUAL(value, 1);

  void *result;
  pthread_join(thread, &result);

  delete mutex;

  return true;
}

static bool MutexLockableTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_MutexLockable,
                                     "test_osc_MutexLockable");
#endif

