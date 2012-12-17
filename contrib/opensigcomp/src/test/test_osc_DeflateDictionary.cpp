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
#include "DeflateDictionary.h"
#include "TestList.h"
#include "Types.h"

osc::byte_t history1[] = "FooBarBaz";
osc::byte_t history2[] = "Abbabbabb";
osc::byte_t future[]   = "BarBazbbaazAbbFonar";

bool test_osc_DeflateDictionary()
{
  unsigned int length;
  unsigned int distance;
  bool found;

  osc::DeflateDictionary dict;

  dict.addHistory (history1, sizeof(history1)-1);
  TEST_ASSERT_EQUAL(dict.getSize(), sizeof(history1)-1);

  dict.addHistory (history2, sizeof(history2)-1);
  TEST_ASSERT_EQUAL(dict.getSize(), sizeof(history1) + sizeof(history2) - 2);
  
  dict.addFuture (future, sizeof(future) - 1);
  TEST_ASSERT_EQUAL(dict.getSize(), sizeof(history1) + 
                                    sizeof(history2) +
                                    sizeof(future) - 3);

  // BarBaz -- from History 1
  found = dict.findNextLengthAndDistance(length, distance);
  TEST_ASSERT(found);
  TEST_ASSERT_EQUAL(length, 6);
  TEST_ASSERT_EQUAL(distance, 15);
  dict.increment(length);

  // bba -- from history 2
  found = dict.findNextLengthAndDistance(length, distance);
  TEST_ASSERT(found);
  TEST_ASSERT_EQUAL(length, 3);
  TEST_ASSERT_EQUAL(distance, 11);
  dict.increment(length);

  // az from history 1, Abb from history 2
  found = dict.findNextLengthAndDistance(length, distance);
  TEST_ASSERT(found);
  TEST_ASSERT_EQUAL(length, 5);
  TEST_ASSERT_EQUAL(distance, 20);
  dict.increment(length);

  // no match ("Fo" is too short to match)
  found = dict.findNextLengthAndDistance(length, distance);
  TEST_ASSERT(!found);
  dict.increment();

  return true;
}

static bool DeflateDictionaryTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_DeflateDictionary,
                                     "test_osc_DeflateDictionary");
