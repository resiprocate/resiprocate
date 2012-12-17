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
#include "State.h"
#include "StateMap.h"
#include "TestList.h"

static osc::byte_t stateA1[] = {192, 204, 63, 238, 121, 188, 252, 143, 209, 8};
static osc::byte_t stateA2[] = {101, 232, 3, 82, 238, 41, 119, 23, 223, 87};

bool test_osc_StateMap()
{
  osc::StateMap map;
  osc::State *s1;
  osc::State *s2;
  osc::State *s3;
  osc::State *a1;
  osc::State *a2;
  osc::State *tmp;

  osc::nack_code_t status;

  s1=new osc::State(osc::Buffer(reinterpret_cast<const osc::byte_t*>("a"),1));
  s2=new osc::State(osc::Buffer(reinterpret_cast<const osc::byte_t*>("b"),1));
  s3=new osc::State(osc::Buffer(reinterpret_cast<const osc::byte_t*>("b"),1));

  // A1 and A2 have state IDs that match in the first 6 characters.
  a1 = new osc::State(osc::Buffer(stateA1,10), 256,0,20);
  a2 = new osc::State(osc::Buffer(stateA2,10), 266,0,20);

  // Verify that A1 and A2 have matching state IDs for the first 6 chars
  osc::Buffer a1id(a1->getId());
  osc::Buffer a2id(a2->getId());
  a1id.truncate(6);
  a2id.truncate(6);
  TEST_ASSERT_EQUAL_BUFFERS(a1id.data(),a2id.data(), 6);

  // Add state 1 -- verify proper return
  s1->retain();
  tmp = map.insert(s1);
  s1->release();
  TEST_ASSERT_EQUAL(tmp, s1);

  // Ensure that we can find state 1 with a full ID
  tmp = 0;
  tmp = map.find(s1->getId(), status);
  TEST_ASSERT_EQUAL(tmp, s1);

  // Ensure that we can find state 1 with a partial ID
  osc::Buffer id1(s1->getId());
  id1.truncate(6);
  tmp = 0;
  tmp = map.find(id1, status);
  TEST_ASSERT_EQUAL(tmp, s1);

  // Ensure that we get a proper error code before we add state 2
  tmp = 0;
  tmp = map.find(s2->getId(), status);
  TEST_ASSERT_EQUAL(tmp, 0);
  TEST_ASSERT_EQUAL(status, osc::STATE_NOT_FOUND);

  // Add state 2 -- verify proper return
  s2->retain();
  tmp = map.insert(s2);
  s2->release();
  TEST_ASSERT_EQUAL(tmp, s2);

  // Ensure that we can find state 2 with a full ID
  tmp = 0;
  tmp = map.find(s2->getId(), status);
  TEST_ASSERT_EQUAL(tmp, s2);

  // Add state 3 -- verify that we get state 2 in return
  s3->retain();
  tmp = map.insert(s3);
  tmp->release();
  TEST_ASSERT_EQUAL(tmp, s2);

  // Ensure that we can still find state 2 with a full ID
  tmp = 0;
  tmp = map.find(s2->getId(), status);
  TEST_ASSERT_EQUAL(tmp, s2);

  // Remove state 1
  tmp = map.remove(s1->getId());
  TEST_ASSERT_EQUAL(tmp, s1);

  // Ensure that we get a proper error code after we remove state 1
  tmp = 0;
  tmp = map.find(s1->getId(), status);
  TEST_ASSERT_EQUAL(tmp, 0);
  TEST_ASSERT_EQUAL(status, osc::STATE_NOT_FOUND);

  // Insert states whose IDs collide in the first 6 bytes
  a1->retain();
  tmp = map.insert(a1);
  a1->release();
  TEST_ASSERT_EQUAL(tmp, a1);
  a2->retain();
  tmp = map.insert(a2);
  a2->release();
  TEST_ASSERT_EQUAL(tmp, a2);

  // Ensure that we get a proper error code for colliding state IDs
  tmp = 0;
  tmp = map.find(a1id, status);
  TEST_ASSERT_EQUAL(tmp, 0);
  TEST_ASSERT_EQUAL(status, osc::ID_NOT_UNIQUE);

  delete(s1);

  return true;
}

static bool StateMapTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_StateMap,
                                     "test_osc_StateMap");
