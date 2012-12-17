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
#include "CrcComputer.h"
#include "TestList.h"

bool test_osc_CrcComputer()
{
  osc::CrcComputer computer;
  osc::u16 crc;
  osc::byte_t buffer[256];
  int i;

  // Set up buffer
  for (i = 0; i <24; i ++)
  {
    buffer[i] = i+1;
  }
  for (i = 0; i < 20; i ++)
  {
    buffer[i+24] = i+128;
  }

  // Calculate CRC
  computer.addData(buffer, 44);
  crc = computer.getCrc();

  // Validate CRC
  TEST_ASSERT_EQUAL(crc, 0x62cb);

  return true;
}

static bool CrcComputerTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_CrcComputer,
                                     "test_osc_CrcComputer");
