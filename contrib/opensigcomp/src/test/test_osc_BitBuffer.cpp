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
#include "BitBuffer.h"
#include "TestList.h"
#include "Types.h"

bool test_osc_BitBuffer()
{
  osc::byte_t buffer[256];
  unsigned int i;

  for (i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = i & 0xFF;
  }

  osc::BitBuffer b1 (buffer, sizeof(buffer), 0, osc::BitBuffer::MSB_FIRST);

  // Verify that the buffer has been zeroed out
  for (i = 0; i < sizeof(buffer); i++)
  {
    if (buffer[i] != 0)
    {
      std::cout << __FILE__ << ':' << __LINE__ 
        << ": buffer[" << i << "] is " << buffer[i]
        << " instead of 0" << std::endl;
      return false;
    }
  }
 
  // appendMsbFirst into MSB buffer
  b1.appendMsbFirst(2, 0x01);
  TEST_ASSERT_EQUAL(buffer[0],0x40);
  b1.appendMsbFirst(2, 0x01);
  TEST_ASSERT_EQUAL(buffer[0],0x50);
  b1.appendMsbFirst(11, 0xf8E>>1);
  TEST_ASSERT_EQUAL(buffer[0],0x5F);
  TEST_ASSERT_EQUAL(buffer[1],0x8e);
  b1.appendMsbFirst(1, 1);
  TEST_ASSERT_EQUAL(buffer[1],0x8f);
  b1.appendMsbFirst(9, 0x1ff);
  TEST_ASSERT_EQUAL(buffer[2],0xff);
  TEST_ASSERT_EQUAL(buffer[3],0x80);
  b1.appendMsbFirst(9, 0x1ff);
  TEST_ASSERT_EQUAL(buffer[3],0xFF);
  TEST_ASSERT_EQUAL(buffer[4],0xC0);

  // appendLsbFirst in to MSB buffer
  b1.padOutputByte(1);
  TEST_ASSERT_EQUAL(buffer[4],0xFF);
  b1.padOutputByte(1);

  b1.appendLsbFirst(11, 0x322);
  TEST_ASSERT_EQUAL(buffer[5],0x44);
  TEST_ASSERT_EQUAL(buffer[6],0xC0);
  b1.padOutputByte(1);
  TEST_ASSERT_EQUAL(buffer[6],0xDF);

  osc::BitBuffer b2 (buffer, sizeof(buffer), 0, osc::BitBuffer::LSB_FIRST);

  // appendMsbFirst in to LSB buffer
  b2.appendMsbFirst(11,0x322);
  TEST_ASSERT_EQUAL(buffer[0],0x26);
  TEST_ASSERT_EQUAL(buffer[1],0x02);

  // appendLsbFirst in to LSB buffer
  b2.padOutputByte(1);
  TEST_ASSERT_EQUAL(buffer[1],0xfa);
  b2.appendLsbFirst(11,0x322);
  TEST_ASSERT_EQUAL(buffer[2],0x22);
  TEST_ASSERT_EQUAL(buffer[3],0x03);
  b2.padOutputByte(1);
  TEST_ASSERT_EQUAL(buffer[3],0xfb);

  // readMsbFirst from MSB buffer
  buffer[0] = 0x5f;
  buffer[1] = 0x8f;
  buffer[2] = 0xff;
  buffer[3] = 0xff;
  buffer[4] = 0xc0;
  osc::BitBuffer b3 (buffer, sizeof(buffer), 35, osc::BitBuffer::MSB_FIRST);
  osc::u16 value;
  value = b3.readMsbFirst(2);
  TEST_ASSERT_EQUAL(value, 1);
  value = b3.readMsbFirst(2);
  TEST_ASSERT_EQUAL(value, 1);
  value = b3.readMsbFirst(11);
  TEST_ASSERT_EQUAL(value, 0xf8e>>1);
  value = b3.readMsbFirst(1);
  TEST_ASSERT_EQUAL(value, 1);
  value = b3.readMsbFirst(9);
  TEST_ASSERT_EQUAL(value, 0x1ff);
  value = b3.readMsbFirst(9);
  TEST_ASSERT_EQUAL(value, 0x1ff);
  value = b3.readMsbFirst(6);
  TEST_ASSERT_EQUAL(value, 0);


  // readLsbFirst from MSB buffer
  buffer[0] = 0x44;
  buffer[1] = 0xDf;
  osc::BitBuffer b4 (buffer, sizeof(buffer), 11, osc::BitBuffer::MSB_FIRST);
  value = b4.readLsbFirst(11);
  TEST_ASSERT_EQUAL(value, 0x322);
  value = b4.readLsbFirst(6);
  TEST_ASSERT_EQUAL(value, 0);

  // readMsbFirst from LSB buffer
  buffer[0] = 0x26;
  buffer[1] = 0xfa;
  osc::BitBuffer b5 (buffer, sizeof(buffer), 11, osc::BitBuffer::LSB_FIRST);
  value = b5.readMsbFirst(11);
  TEST_ASSERT_EQUAL(value, 0x322);
  value = b5.readMsbFirst(6);
  TEST_ASSERT_EQUAL(value, 0);

  // readLsbFirst from LSB buffer
  buffer[0] = 0x22;
  buffer[1] = 0xfb;
  osc::BitBuffer b6 (buffer, sizeof(buffer), 11, osc::BitBuffer::LSB_FIRST);
  value = b6.readLsbFirst(11);
  TEST_ASSERT_EQUAL(value, 0x322);
  value = b6.readLsbFirst(6);
  TEST_ASSERT_EQUAL(value, 0);

  // appendBytes
  osc::BitBuffer b7 (buffer, sizeof(buffer), 0, osc::BitBuffer::MSB_FIRST);
  osc::byte_t tmp[] = {0x5f, 0x8f};
  TEST_ASSERT(b7.appendBytes(tmp, sizeof(tmp)));
  value = b7.readMsbFirst(4); TEST_ASSERT_EQUAL(value, 0x05);
  value = b7.readMsbFirst(4); TEST_ASSERT_EQUAL(value, 0x0f);
  value = b7.readMsbFirst(4); TEST_ASSERT_EQUAL(value, 0x08);
  value = b7.readMsbFirst(4); TEST_ASSERT_EQUAL(value, 0x0f);

  // readBytes
  osc::BitBuffer b8 (buffer, sizeof(buffer), 0, osc::BitBuffer::MSB_FIRST);
  b8.appendMsbFirst(4, 0x09);
  b8.appendMsbFirst(4, 0x06);
  b8.appendMsbFirst(4, 0x0A);
  b8.appendMsbFirst(4, 0x03);
  TEST_ASSERT(b8.readBytes(tmp, 2));
  TEST_ASSERT_EQUAL(tmp[0],0x96);
  TEST_ASSERT_EQUAL(tmp[1],0xA3);
  
  return true;
}

static bool BitBufferTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_BitBuffer,
                                     "test_osc_BitBuffer");
