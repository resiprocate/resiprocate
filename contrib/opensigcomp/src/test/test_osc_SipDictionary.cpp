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
#include "SipDictionary.h"
#include "TestList.h"

bool test_osc_SipDictionary()
{
  osc::SipDictionary* state = new osc::SipDictionary();

  // std::cout << *state << std::endl;

  // Test string location by offset
  {
    state->buildOffsetTable();
    size_t offset, length;
    bool found;
    osc::byte_t *invite = (osc::byte_t *)"INVITE ";
    osc::byte_t *fnoo = (osc::byte_t *)"FNOO ";
    osc::byte_t *invitation = (osc::byte_t *)"INVITATION ";

    // Look for "INVITE " in the buffer
    found = state->findOffset(invite, 7, offset, length);
    TEST_ASSERT_EQUAL(found, true);
    TEST_ASSERT_EQUAL(offset, 3406);
    TEST_ASSERT_EQUAL(length, 7);

    // Look for "FNOO " in the buffer
    found = state->findOffset(fnoo, 5, offset, length);
    TEST_ASSERT_EQUAL(found, false);

    // Look for "INVITATION " in the buffer
    found = state->findOffset(invitation, 11, offset, length);
    TEST_ASSERT_EQUAL(found, true);
    TEST_ASSERT_EQUAL(offset, 3406);
    TEST_ASSERT_EQUAL(length, 5);
  }

  // Test string location by index
  {
    state->buildIndexTable();
    size_t index, length;
    bool found;
    osc::byte_t *invite = (osc::byte_t *)"INVITE x";
    osc::byte_t *fnoo = (osc::byte_t *)"FNOO ";
    osc::byte_t *invitation = (osc::byte_t *)"INVITATION ";

    // Look for "INVITE " in the buffer
    found = state->findIndex(invite, 7, index, length);
    TEST_ASSERT_EQUAL(found, true);
    TEST_ASSERT_EQUAL(index, 7);
    TEST_ASSERT_EQUAL(length, 7);

    // Look for "INVITE" in the buffer
    found = state->findIndex(invite, 6, index, length);
    TEST_ASSERT_EQUAL(found, true);
    TEST_ASSERT_EQUAL(index, 6);
    TEST_ASSERT_EQUAL(length, 6);

    // Look for "INVITE x" in the buffer
    found = state->findIndex(invite, 8, index, length);
    TEST_ASSERT_EQUAL(found, true);
    TEST_ASSERT_EQUAL(index, 7);
    TEST_ASSERT_EQUAL(length, 7);

    // Look for "FNOO " in the buffer
    found = state->findIndex(fnoo, 5, index, length);
    TEST_ASSERT_EQUAL(found, false);

    // Look for "INVITATION " in the buffer
    found = state->findIndex(invitation, 11, index, length);
    TEST_ASSERT_EQUAL(found, false);
  }

  /*
   Name:                    Value:
   =====================    ========================
   state_identifier         0xfbe507dfe5e6aa5af2abb914ceaa05f99ce61ba5
   state_length             0x12E4
   state_address            0 (not relevant for the dictionary)
   state_instruction        0 (not relevant for the dictionary)
   minimum_access_length    6
  */

  TEST_ASSERT_EQUAL(state->getStateSize(), 0x12e4);
  TEST_ASSERT_EQUAL(state->getAddress(), 0);
  TEST_ASSERT_EQUAL(state->getInstruction(), 0);
  TEST_ASSERT_EQUAL(state->getMinimumAccessLength(), 6);

  osc::state_id_t id = state->getId();
  TEST_ASSERT_EQUAL(id[0], 0xfb);
  TEST_ASSERT_EQUAL(id[1], 0xe5);
  TEST_ASSERT_EQUAL(id[2], 0x07);
  TEST_ASSERT_EQUAL(id[3], 0xdf);
  TEST_ASSERT_EQUAL(id[4], 0xe5);
  TEST_ASSERT_EQUAL(id[5], 0xe6);
  TEST_ASSERT_EQUAL(id[6], 0xaa);
  TEST_ASSERT_EQUAL(id[7], 0x5a);
  TEST_ASSERT_EQUAL(id[8], 0xf2);
  TEST_ASSERT_EQUAL(id[9], 0xab);
  TEST_ASSERT_EQUAL(id[10], 0xb9);
  TEST_ASSERT_EQUAL(id[11], 0x14);
  TEST_ASSERT_EQUAL(id[12], 0xce);
  TEST_ASSERT_EQUAL(id[13], 0xaa);
  TEST_ASSERT_EQUAL(id[14], 0x05);
  TEST_ASSERT_EQUAL(id[15], 0xf9);
  TEST_ASSERT_EQUAL(id[16], 0x9c);
  TEST_ASSERT_EQUAL(id[17], 0xe6);
  TEST_ASSERT_EQUAL(id[18], 0x1b);
  TEST_ASSERT_EQUAL(id[19], 0xa5);

  delete(state);

  return true;
}

static bool SipDictionaryTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_SipDictionary,
                                     "test_osc_SipDictionary");
