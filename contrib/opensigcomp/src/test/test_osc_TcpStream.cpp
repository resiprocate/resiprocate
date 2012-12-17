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
#include "TcpStream.h"
#include "TestList.h"

osc::byte_t stream1[]=
{
  0xF8 | 0x01, // header
  1,2,3,4,5,6, // state ID
  1,2,3,4,5,6,7,8,9,10, // Input
  0xFF, 0xFF, // terminator

  0xF8, // header
  0x01, // Length, high-byte (0x12 = 18)
  0x22, // Length, low nybble, destination (2 => 192)
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,// bytecodes
  1,2,3,4,5,6,7,8,9,10, // Input
  0xFF, 0xFF // terminator
};

osc::byte_t stream2a[]=
{
  0xFF, 0x00,
  0x83, 0xFF,0x00, 0xFF,0x00 ,0xFF,0x00, // feedback
  // State ID
  0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
  0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
  // Input
  0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
  0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
  // Terminator
  0xFF, 0xFF
};

osc::byte_t stream2b[]=
{
  0xFF, 26,
  0x83,0xFF,0xFF,0xFF, // feedback
  // State ID
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  // Input
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  // Terminator
  0xFF, 0xFF
};

osc::byte_t stream2dg[] =
{
  0xF8 | 0x04 | 0x03, // header
  0x83,0xFF,0xFF,0xFF, // feedback
  // State ID
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  // Input
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

osc::byte_t framingError[] = {0xff, 0x80};

bool test_osc_TcpStream()
{
  osc::TcpStream stream;
  osc::SigcompMessage *sm;
  osc::TcpStream::status_t status;
  size_t i;

  // Simple two-message test
  status = stream.addData(stream1, sizeof(stream1));
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);

  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT(!sm->getReturnedFeedback());
  TEST_ASSERT_EQUAL(sm->getStateIdLength(), 6);
  for (i = 0; i < sm->getStateIdLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getStateId()[i], i+1);
  }
  TEST_ASSERT_EQUAL(sm->getInputLength(), 10);
  for (i = 0; i < sm->getInputLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getInput()[i], i+1);
  }
  TEST_ASSERT(!sm->getBytecodes());

  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);

  // Try it with the last character missing
  status = stream.addData(stream1, sizeof(stream1) - 1);
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);
  status = stream.addData(stream1 + sizeof(stream1) - 1, 1);
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);

  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT(!sm->getReturnedFeedback());
  TEST_ASSERT_EQUAL(sm->getStateIdLength(), 0);
  TEST_ASSERT(!sm->getStateId());

  TEST_ASSERT_EQUAL(sm->getBytecodeLength(), 18);
  TEST_ASSERT_EQUAL(sm->getBytecodeDestination(), 192);
  for (i = 0; i < 18; i++)
  {
    TEST_ASSERT_EQUAL(sm->getBytecodes()[i], i+1);
  }
  TEST_ASSERT_EQUAL(sm->getInputLength(), 10);
  for (i = 0; i < sm->getInputLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getInput()[i], i+1);
  }

  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);

  // Now with two writes after the initial one
  status = stream.addData(stream1, sizeof(stream1) - 2);
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);
  status = stream.addData(stream1 + sizeof(stream1) - 2, 1);
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);

  status = stream.addData(stream1 + sizeof(stream1) - 1, 1);
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);

  // Degenerate case: each character added one-at-a-time
  for (i = 0; i < sizeof(stream1); i++)
  {
    status = stream.addData(stream1+i, 1);
    TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  }
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  delete(sm);
  sm = stream.getNextMessage();
  TEST_ASSERT(!sm);

  // Try silly escaping
  status = stream.addData(stream2a, sizeof(stream2a));
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(stream2dg));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(), stream2dg, 
                            sizeof(stream2dg));
  delete(sm);

  // Now, sensible escaping
  status = stream.addData(stream2b, sizeof(stream2b));
  TEST_ASSERT_EQUAL(status, osc::TcpStream::OK);
  sm = stream.getNextMessage();
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(stream2dg));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(), stream2dg, 
                            sizeof(stream2dg));
  delete(sm);

  // Finally, a framing error
  status = stream.addData(framingError, sizeof(framingError));
  TEST_ASSERT_EQUAL(status, osc::TcpStream::FRAMING_ERROR);

  return true;
}

static bool TcpStreamTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_TcpStream,
                                     "test_osc_TcpStream");
