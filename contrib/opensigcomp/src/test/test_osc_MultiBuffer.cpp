/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2006 Estacado Systems, LLC

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
#include "MultiBuffer.h"
#include "TestList.h"

osc::byte_t a1[] = {1,2,3,4,5};
osc::byte_t a2[] = {6,7,8,9,10};
osc::byte_t a3[] = {11,12,13,14,15};

bool test_osc_MultiBuffer()
{
  osc::MultiBuffer mb;
  osc::Buffer b3(a3,sizeof(a3));

  mb.addBlock(a1, sizeof(a1));
  TEST_ASSERT_EQUAL(mb.getSize(), sizeof(a1));
  mb.addBlock(a2, sizeof(a2));
  TEST_ASSERT_EQUAL(mb.getSize(), sizeof(a1) + sizeof(a2));
  mb.addBlock(b3);
  TEST_ASSERT_EQUAL(mb.getSize(), sizeof(a1) + sizeof(a2) + sizeof(a3));
  mb.addBlock(a1, sizeof(a1));
  TEST_ASSERT_EQUAL(mb.getSize(), sizeof(a1)*2 + sizeof(a2) + sizeof(a3));

  for (int i = 0; i < 15; i++)
  {
    TEST_ASSERT_EQUAL(mb[i],i+1);
  }

  for (int j = 15; j < 20; j++)
  {
    TEST_ASSERT_EQUAL(mb[j],j-14);
  }

  return true;
}

static bool MultiBufferTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_MultiBuffer,
                                     "test_osc_MultiBuffer");
