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
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */


#include <iostream>
#include "StateList.h"
#include "TestList.h"
#include "State.h"

bool test_osc_StateList()
{
  size_t i;
  osc::StateList slist(4096);

  // KEEP IN MIND that each state take its own size PLUS 64 BYTES.

  TEST_ASSERT_EQUAL(slist.numberOfStates(),0);
  TEST_ASSERT_EQUAL(slist.getQuota(),4096);

  osc::byte_t bytes1[] = {0,1,2,3,4};
  osc::Buffer buff1(bytes1,sizeof(bytes1));
  osc::State s1(buff1);

  // Test single add
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  slist.addState(&s1, 1, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);

  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096 - sizeof(bytes1) - 64);

  // Test reset
  slist.reset();
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);

  // Test double add (same priority)
  slist.addState(&s1, 1, false);
  slist.addState(&s1, 1, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096 - sizeof(bytes1) - 64);

  // Test multiple add (lower priority)
  slist.addState(&s1, 2, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096 - sizeof(bytes1) - 64);

  // Test multiple add (higher priority)
  slist.addState(&s1, 0, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096 - sizeof(bytes1) - 64);

  // Test adding second state
  osc::byte_t bytes2[] = {0,2,2,3,4};
  osc::Buffer buff2(bytes2,sizeof(bytes2));
  osc::State s2(buff2);
  slist.addState(&s2, 1, false);
  TEST_ASSERT_EQUAL(s2.getRefCount(),1);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),
                    4096 - sizeof(bytes1) - sizeof(bytes2) - 128);

  // Test adding big state that removes other two
  osc::byte_t bigBytes1[4096 - 64];
  for (i = 0; i < sizeof(bigBytes1); i++)
  {
    bigBytes1[i] = (i & 0xff);
  }
  osc::Buffer bigBuff1(bigBytes1, sizeof(bigBytes1));
  osc::State bigState1(bigBuff1);
  slist.addState(&bigState1, 0, false);
  TEST_ASSERT_EQUAL(bigState1.getRefCount(),1);
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  TEST_ASSERT_EQUAL(s2.getRefCount(),0);

  // Test adding state that bumps exactly one state out (based on time).
  slist.reset();
  osc::Buffer bigBuff2(bigBytes1, sizeof(bigBytes1) - 64 - sizeof(bytes1));
  osc::State bigState2(bigBuff2);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096);
  slist.addState(&s2, 0, false);
  slist.addState(&s1, 0, false);
  slist.addState(&bigState2, 0, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);
  TEST_ASSERT_EQUAL(s2.getRefCount(),0);
  TEST_ASSERT_EQUAL(bigState2.getRefCount(),1);

  // Lowest priority = freed first

  // Test adding state that bumps exactly one state out (based on priority).
  slist.reset();
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096);
  slist.addState(&s2, 1, false);
  slist.addState(&s1, 0, false);
  slist.addState(&bigState2, 0, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  TEST_ASSERT_EQUAL(s2.getRefCount(),1);
  TEST_ASSERT_EQUAL(bigState2.getRefCount(),1);

  // Same test, but with state reprioritization
  slist.reset();
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096);
  slist.addState(&s2, 0, false);
  slist.addState(&s1, 1, false);
  slist.addState(&s2, 2, false);
  slist.addState(&bigState2, 0, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  TEST_ASSERT_EQUAL(s2.getRefCount(),1);
  TEST_ASSERT_EQUAL(bigState2.getRefCount(),1);

  // Make sure insertion at a lower priority doesn't downgrade
  slist.reset();
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096);
  slist.addState(&s2, 2, false);
  slist.addState(&s1, 1, false);
  slist.addState(&s2, 0, false);
  slist.addState(&bigState2, 0, false);
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  TEST_ASSERT_EQUAL(s2.getRefCount(),1);
  TEST_ASSERT_EQUAL(bigState2.getRefCount(),1);

  // Now, bump out the big state by lowering the quota
  slist.setQuota(2048);
  TEST_ASSERT_EQUAL(s1.getRefCount(),0);
  TEST_ASSERT_EQUAL(s2.getRefCount(),1);
  TEST_ASSERT_EQUAL(bigState2.getRefCount(),0);
  slist.setQuota(4096);

  // Try removing a state manually
  slist.reset();
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096);
  slist.addState(&s2, 2, false);
  slist.addState(&s1, 1, false);
  slist.removeState(s2.getStateId());
  TEST_ASSERT_EQUAL(s1.getRefCount(),1);
  TEST_ASSERT_EQUAL(s2.getRefCount(),0);
  TEST_ASSERT_EQUAL(slist.getQuotaLeft(),4096 - sizeof(bytes1) - 64);

  // Insert a pre-acked state, check acked state retreival
  slist.addState(&s2, 1, true);
  slist.addState(&s1, 1, false);
  TEST_ASSERT(!(slist.isStateAcked(s1.getStateId())));
  TEST_ASSERT(slist.isStateAcked(s2.getStateId()));
  TEST_ASSERT_EQUAL(slist.getMostRecentAckedState(1),&s2);
  slist.ackState(s1.getStateId());
  TEST_ASSERT_EQUAL(slist.getMostRecentAckedState(1),&s1);
  slist.addState(&s2, 1, false);
  TEST_ASSERT_EQUAL(slist.getMostRecentAckedState(1),&s2);

  TEST_ASSERT_EQUAL(slist.getMostRecentAckedState(0),0);
  TEST_ASSERT_EQUAL(slist.getMostRecentAckedState(2),0);

  return true;
}

static bool StateListTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_StateList,
                                     "test_osc_StateList");
