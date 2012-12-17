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
#include "DeflateData.h"
#include "TestList.h"


bool test_osc_DeflateData()
{
  int i, j;
  osc::DeflateData *dd = new osc::DeflateData();
  TEST_ASSERT(dd);
  osc::CompressorData *cd = dd;

  TEST_ASSERT_EQUAL(cd->getDataType(),osc::DeflateCompressor::COMPRESSOR_ID);

  // Set up 17 ids for testing
  osc::byte_t id[17][2];
  osc::Buffer bid[17];
  for (i = 0; i < 17; i++)
  {
    id[i][0] = i+1;
    id[i][1] = i+1;
    bid[i].copy(id[i],2);
  }

  osc::sha1_t hash;

  TEST_ASSERT_EQUAL(dd->getCurrStateSerial(), 15);
  for (i = 0; i < 17; i++)
  {
    TEST_ASSERT_EQUAL(dd->getStateId(i).size(), 0);
  }

  for (i = 0; i < 16; i++)
  {
    dd->incrementSerial();
    dd->storeCurrStateId(bid[i]);
    for (j = 0; j < 20; j++)
    {
      hash.digest[j] = i+j;
    }
    dd->storeCurrNackHash(hash);

    // Make sure we can find by hash value.
    signed int index = dd->findNackHash(hash);
    TEST_ASSERT_EQUAL(index, i);
    index = dd->findNackHash(hash);
    TEST_ASSERT_EQUAL(index, -1);
    TEST_ASSERT_EQUAL(dd->getStateId(i)[0], i+1);
    TEST_ASSERT_EQUAL(dd->getCurrStateSerial(), (i)%16);
  }

  dd->incrementSerial();
  dd->storeCurrStateId(bid[16]);
  TEST_ASSERT_EQUAL(dd->getStateId(i)[0], 17);
  TEST_ASSERT_EQUAL(dd->getCurrStateSerial(), 0);

  for (i = 1; i < 16; i++)
  {
    TEST_ASSERT_EQUAL(dd->getStateId(i)[0], i+1);
  }

  delete cd;
  return true;
}

static bool DeflateDataTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_DeflateData,
                                     "test_osc_DeflateData");
