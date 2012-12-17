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
#include "SigcompMessage.h"
#include "TestList.h"
#include "Sha1Hasher.h"

// Partial state ID variant (length = 6), no feedback
osc::byte_t message1[] =
{
  0xF8 | 0x01, // header
  1,2,3,4,5,6, // state ID
  1,2,3,4,5,6,7,8,9,10 // Input
};

// Bytecode variant, no feedback
osc::byte_t message2[] =
{
  0xF8, // header
  0x01, // Length, high-byte (0x12 = 18)
  0x22, // Length, low nybble, destination (2 => 192)
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,// bytecodes
  1,2,3,4,5,6,7,8,9,10 // Input
};

// Partial state ID variant (length = 9), with 1-byte feedback
osc::byte_t message3[] =
{
  0xF8 | 0x04 | 0x02, // header
  0x7f, // feedback
  1,2,3,4,5,6,7,8,9, // state ID
  1,2,3,4,5,6,7,8,9,10 // Input
};

// Partial state ID variant (length = 12), with multi-byte feedback
osc::byte_t message4[] =
{
  0xF8 | 0x04 | 0x03, // header
  0x83,1,2,3, // feedback
  1,2,3,4,5,6,7,8,9,10,11,12, // state ID
  1,2,3,4,5,6,7,8,9,10 // Input
};

// NACK format, no feedback
osc::byte_t message5[] =
{
  0xF8, // header
  0x00, // Length, high-byte
  0x01, // Length, low nybble, version
  2,    // Cycles Exhausted
  1,    // Failed opcode
  1,128, // PC of failed instruction
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20, // SHA-1 ID
  64 // Error details
};

// NACK format, with feedback
osc::byte_t message6[] =
{
  0xF8 | 0x04, // header
  0x83,1,2,3, // feedback
  0x00, // Length, high-byte
  0x01, // Length, low nybble, version
  2,    // Cycles Exhausted
  1,    // Failed opcode
  1,128, // PC of failed instruction
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20, // SHA-1 ID
  64 // Error details
};

// Underflow
osc::byte_t message7[] =
{
  0xF8 | 0x01, // header
  1,2,3,4,5  // state ID (truncated)
};

// Lots of 0xFFs
osc::byte_t message8[] =
{
  0xF8 | 0x04 | 0x03, // header
  0x83,0xFF,0xFF,0xFF, // feedback
  // State ID
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
  // Input
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// XXX This is based on the current, braindead method of escaping.
//     It should be adjusted once we optimize TCP message handling.
osc::byte_t message8escaped[] =
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

osc::byte_t bytecodes[] =
{
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17
};

osc::byte_t input[] =
{
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6
};

osc::byte_t stateId[] =
{
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
  0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b
};

osc::byte_t feedback[] =
{
  0x83,1,2,3
};

bool test_osc_SigcompMessage()
{
  osc::SigcompMessage *sm;
  size_t i;

  //////////////////////////////////////////////////////////////////////
  // Test "from wire" constructor

  ////////////
  // Message 1
  sm = new osc::SigcompMessage(message1, sizeof(message1));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message1));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message1,sizeof(message1));
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
  delete sm;

  ////////////
  // Message 2
  sm = new osc::SigcompMessage(message2, sizeof(message2));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message2));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message2,sizeof(message2));
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
  delete sm;

  ////////////
  // Message 3
  sm = new osc::SigcompMessage(message3, sizeof(message3));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message3));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message3,sizeof(message3));
  TEST_ASSERT(sm->getReturnedFeedback());
  TEST_ASSERT_EQUAL(sm->getReturnedFeedbackLength(), 1);
  TEST_ASSERT_EQUAL(*(sm->getReturnedFeedback()), 0x7f);
  TEST_ASSERT_EQUAL(sm->getStateIdLength(), 9);
  for (i = 0; i < sm->getStateIdLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getStateId()[i], i+1);
  }
  TEST_ASSERT(!sm->getBytecodes());
  TEST_ASSERT_EQUAL(sm->getInputLength(), 10);
  for (i = 0; i < sm->getInputLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getInput()[i], i+1);
  }
  delete sm;
  

  ////////////
  // Message 4
  sm = new osc::SigcompMessage(message4, sizeof(message4));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message4));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message4,sizeof(message4));
  TEST_ASSERT(sm->getReturnedFeedback());
  TEST_ASSERT_EQUAL(sm->getReturnedFeedbackLength(), 4);

  TEST_ASSERT_EQUAL(sm->getReturnedFeedback()[0], 0x83);
  TEST_ASSERT_EQUAL(sm->getReturnedFeedback()[1], 1);
  TEST_ASSERT_EQUAL(sm->getReturnedFeedback()[2], 2);
  TEST_ASSERT_EQUAL(sm->getReturnedFeedback()[3], 3);

  TEST_ASSERT_EQUAL(sm->getStateIdLength(), 12);
  for (i = 0; i < sm->getStateIdLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getStateId()[i], i+1);
  }
  TEST_ASSERT(!sm->getBytecodes());
  TEST_ASSERT_EQUAL(sm->getInputLength(), 10);
  for (i = 0; i < sm->getInputLength(); i++)
  {
    TEST_ASSERT_EQUAL(sm->getInput()[i], i+1);
  }
  delete sm;

  ////////////
  // Message 5
  sm = new osc::SigcompMessage(message5, sizeof(message5));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message5));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message5,sizeof(message5));
  TEST_ASSERT_EQUAL(sm->getNackReason(), osc::CYCLES_EXHAUSTED);
  TEST_ASSERT_EQUAL(sm->getNackOpcode(), 1);
  TEST_ASSERT_EQUAL(sm->getNackPc(), 256 + 128);
  for (i = 0; i < 20; i++)
  {
    TEST_ASSERT_EQUAL(sm->getNackSha1().digest[i], i+1);
  }
  TEST_ASSERT_EQUAL(sm->getNackDetailLength(), 1);
  TEST_ASSERT_EQUAL(*(sm->getNackDetails()), 64);
  
  delete (sm);

  ////////////
  // Message 6
  sm = new osc::SigcompMessage(message6, sizeof(message6));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(sm->isNack());
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), sizeof(message6));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage(),message6,sizeof(message6));
  TEST_ASSERT_EQUAL(sm->getNackReason(), osc::CYCLES_EXHAUSTED);
  TEST_ASSERT_EQUAL(sm->getNackOpcode(), 1);
  TEST_ASSERT_EQUAL(sm->getNackPc(), 256 + 128);
  for (i = 0; i < 20; i++)
  {
    TEST_ASSERT_EQUAL(sm->getNackSha1().digest[i], i+1);
  }
  TEST_ASSERT_EQUAL(sm->getNackDetailLength(), 1);
  TEST_ASSERT_EQUAL(*(sm->getNackDetails()), 64);
  delete (sm);
  
  ////////////
  // Message 7 (intentional underflow)
  sm = new osc::SigcompMessage(message7, sizeof(message7));
  TEST_ASSERT(sm);
  TEST_ASSERT(!sm->isValid());
  delete (sm);

  //////////////////////////////////////////////////////////////////////
  // Test Bytecode Constructor
  sm = new osc::SigcompMessage(bytecodes, sizeof(bytecodes), 
                               128, sizeof(input) * 2);
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT(!sm->getReturnedFeedback());
  TEST_ASSERT(!sm->getReturnedFeedbackLength());
  TEST_ASSERT(!sm->getStateId());
  TEST_ASSERT(sm->getInput());
  TEST_ASSERT_EQUAL(sm->getInputLength(),sizeof(input) * 2);

  memmove(sm->getInput(), input, sizeof(input));
  sm->setInputLength(sizeof(input));
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getInputLength(),sizeof(input));

  TEST_ASSERT_EQUAL_BUFFERS(sm->getInput(), input, sizeof(input));

  TEST_ASSERT_EQUAL(sm->getBytecodeLength(), sizeof(bytecodes));
  TEST_ASSERT_EQUAL(sm->getBytecodeDestination(), 128);

  // Test serialization
  TEST_ASSERT_EQUAL(sm->getDatagramLength(),
                    3 + sizeof(bytecodes) + sizeof(input));

  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[0], 0xF8);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[1], sizeof(bytecodes) >> 4);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[2], 
                    ((sizeof(bytecodes) << 4) | 0x01) & 0xFF);

  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3,
                            bytecodes, sizeof(bytecodes));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3+sizeof(bytecodes),
                            input, sizeof(input));
  delete(sm);
  

  //////////////////////////////////////////////////////////////////////
  // Test StateID Constructor
  sm = new osc::SigcompMessage(stateId, 12, sizeof(input) * 2);
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(!sm->isNack());
  TEST_ASSERT(!sm->getReturnedFeedback());
  TEST_ASSERT(!sm->getReturnedFeedbackLength());
  TEST_ASSERT(!sm->getBytecodes());

  memmove(sm->getInput(), input, sizeof(input));
  sm->setInputLength(sizeof(input));
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getInputLength(),sizeof(input));

  TEST_ASSERT_EQUAL_BUFFERS(sm->getInput(), input, sizeof(input));

  TEST_ASSERT_EQUAL(sm->getStateIdLength(), 12);
  TEST_ASSERT_EQUAL_BUFFERS(sm->getStateId(), stateId, 12);

  // Test serialization
  TEST_ASSERT_EQUAL(sm->getDatagramLength(),
                    1 + 12 + sizeof(input));

  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[0], 0xF8 | 0x03);
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+1, stateId,
                            sm->getStateIdLength());
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+13, input, sizeof(input));


  delete(sm);

  //////////////////////////////////////////////////////////////////////
  // Test NACK Constructor
  osc::byte_t details = 64;
  osc::byte_t hash[20];
  osc::SigcompMessage *sm2;
  sm2 = new osc::SigcompMessage(message1, sizeof(message1));
  TEST_ASSERT(sm2);
  TEST_ASSERT(sm2->isValid());

  osc::Sha1Hasher hasher;
  hasher.addData(message1, sizeof(message1));
  hasher.getHash(hash);
  sm = new osc::SigcompMessage(osc::CYCLES_EXHAUSTED, 4, 256+128,
                               *sm2, &details, sizeof(details));
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT(sm->isNack());
  TEST_ASSERT_EQUAL(sm->getNackReason(),osc::CYCLES_EXHAUSTED);
  TEST_ASSERT_EQUAL(sm->getNackOpcode(),4);
  TEST_ASSERT_EQUAL(sm->getNackPc(),256+128);
  TEST_ASSERT_EQUAL(sm->getNackDetailLength(), sizeof(details));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getNackDetails(), &details, sizeof(details));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getNackSha1().digest, hash, 20);

  // Test serialization
  TEST_ASSERT_EQUAL(sm->getDatagramLength(), 27 + sizeof(details));

  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[0], 0xF8); // header
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[1], 0); // code len
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[2], 1); // version
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[3], osc::CYCLES_EXHAUSTED);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[4], 4);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[5], 0x01);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[6], 128);
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+7, hash, 20);
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+27, 
                            &details, sizeof(details));
  delete (sm);
  delete (sm2);

  //////////////////////////////////////////////////////////////////////
  // Test Adding Feedback

  sm = new osc::SigcompMessage(bytecodes, sizeof(bytecodes), 
                               128, sizeof(input) * 2);
  TEST_ASSERT(sm);
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getInputLength(),sizeof(input) * 2);

  memmove(sm->getInput(), input, sizeof(input));
  sm->setInputLength(sizeof(input));
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getInputLength(),sizeof(input));

  TEST_ASSERT_EQUAL_BUFFERS(sm->getInput(), input, sizeof(input));

  TEST_ASSERT_EQUAL(sm->getBytecodeLength(), sizeof(bytecodes));
  TEST_ASSERT_EQUAL(sm->getBytecodeDestination(), 128);

  // Test serialization
  TEST_ASSERT_EQUAL(sm->getDatagramLength(),
                    3 + sizeof(bytecodes) + sizeof(input));

  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[0], 0xF8);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[1], sizeof(bytecodes) >> 4);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[2], 
                    ((sizeof(bytecodes) << 4) | 0x01) & 0xFF);

  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3,
                            bytecodes, sizeof(bytecodes));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3+sizeof(bytecodes),
                            input, sizeof(input));

  // Add the feedback
  sm->addReturnedFeedback(feedback, sizeof(feedback));
  TEST_ASSERT_EQUAL(sm->getReturnedFeedbackLength(), sizeof(feedback));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getReturnedFeedback(),
                            feedback, sizeof(feedback));

  // Re-test serialization
  TEST_ASSERT_EQUAL(sm->getDatagramLength(),
                    3 + sizeof(feedback) + sizeof(bytecodes) + sizeof(input));

  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[0], 0xF8 | 0x04);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[1+sizeof(feedback)], 
                                             sizeof(bytecodes) >> 4);
  TEST_ASSERT_EQUAL(sm->getDatagramMessage()[2+sizeof(feedback)], 
                    ((sizeof(bytecodes) << 4) | 0x01) & 0xFF);

  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3+sizeof(feedback),
                            bytecodes, sizeof(bytecodes));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getDatagramMessage()+3+sizeof(feedback)
                            +sizeof(bytecodes), input, sizeof(input));
  delete(sm);

  //////////////////////////////////////////////////////////////////////
  // Test TCP Escaping
  sm = new osc::SigcompMessage(message8, sizeof(message8));
  TEST_ASSERT(sm->isValid());
  TEST_ASSERT_EQUAL(sm->getStreamLength(), sizeof(message8escaped));
  TEST_ASSERT_EQUAL_BUFFERS(sm->getStreamMessage(), 
                            message8escaped, sizeof(message8escaped));
  delete(sm);

  return true;
}

static bool SigcompMessageTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_SigcompMessage,
                                     "test_osc_SigcompMessage");
