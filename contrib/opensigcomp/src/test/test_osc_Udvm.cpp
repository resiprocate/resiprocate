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
   Boston, MA 021110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */


#include <iostream>

#include "Udvm.h"
#include "StateChanges.h"
#include "UdvmOpcodes.h"
#include "StateHandler.h"
#include "SigcompMessage.h"
#include "TestList.h"
#include "torture_tests.h"

#include "TcpStream.h"
#include "SipDictionary.h"
#include "NackCodes.h"

namespace osc
{

bool test_osc_Udvm_paramDecode()
{
  bool success = true;
  osc::StateHandler stateHandler(8192, 64, 8192, 2);
  osc::Udvm udvm(stateHandler, 65536);

  osc::byte_t params[] = {0x63,
                          0x80 | 0x25, 0x23,
                          0xc0, 0xf1, 0x75,
                          0x01,
                          0x80 | 0x00, 0x02,
                          0xc0, 0x00, 0x03,
                          0x15,
                          0x40 | 0x03, // 0x7501
                          0x86, // 64
                          0x87, // 128
                          0x88, // 256
                          0x8f, // 32768
                          0xE0 | 0x04, // 0x04 + 65504
                          0x90 | 0x02, 0xf3, // 0x02f3 + 61440
                          0x90 | 0x20, 0xf3, // 0x20f3 + 61440
                          0xa0 | 0x1f, 0xff, // 8191
                          0xc0 | 0x00, 0x08, // 0x02c0
                          0x80, 0xbe, 0xef, // 0xbeef
                          0x81, 0x00, 15, // 0x8788
                          0x86
                         };

  osc::u16 value;

  memmove(udvm.m_memory,params,sizeof(params));
  udvm.m_nextParameter = 0;

  //==========================
  // Test Literal Parameters
  //==========================

  // 0nnnnnnn                        N                   0 - 127
  value = udvm.getLiteralParameter();
  if (value != 0x63 || udvm.m_nextParameter != 1)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }
  
  // 10nnnnnn nnnnnnnn               N                   0 - 16383
  value = udvm.getLiteralParameter();
  if (value != 0x2523)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode; expected " <<
              0x2523 << ", got " << value << std::endl;
    success = false;
  }

  // 11000000 nnnnnnnn nnnnnnnn      N                   0 - 65535
  value = udvm.getLiteralParameter();
  if (value != 0xf175)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }


  //==========================
  // Test Reference Parameters
  //==========================

  // 0nnnnnnn                        memory[2 * N]       0 - 65535
  value = udvm.getReferenceParameter();
  if (udvm.getWord(value) != 0x23c0)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 10nnnnnn nnnnnnnn               memory[2 * N]       0 - 65535
  value = udvm.getReferenceParameter();
  if (udvm.getWord(value) != 0xf175)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 11000000 nnnnnnnn nnnnnnnn      memory[N]           0 - 65535
  value = udvm.getReferenceParameter();
  if (udvm.getWord(value) != 0xc0f1)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }


  //==========================
  // Test Multitype Parameters
  //==========================

  // 00nnnnnn                        N                   0 - 63
  value = udvm.getMultitypeParameter();
  if (value != 0x15)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 01nnnnnn                        memory[2 * N]       0 - 65535
  value = udvm.getMultitypeParameter();
  if (value != 0x0180)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode " << std::endl;
    success = false;
  }

  // 1000011n                        2 ^ (N + 6)        64 , 128
  value = udvm.getMultitypeParameter();
  if (value != 64)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  value = udvm.getMultitypeParameter();
  if (value != 128)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 10001nnn                        2 ^ (N + 8)    256 , ... , 32768
  value = udvm.getMultitypeParameter();
  if (value != 256)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  value = udvm.getMultitypeParameter();
  if (value != 32768)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 111nnnnn                        N + 65504       65504 - 65535
  value = udvm.getMultitypeParameter();
  if (value != 0x04 + 65504)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 1001nnnn nnnnnnnn               N + 61440       61440 - 65535
  value = udvm.getMultitypeParameter();
  if (value != ((0x02f3 + 61440) % 0x10000))
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }
  
  // 101nnnnn nnnnnnnn               N                   0 - 8191
  value = udvm.getMultitypeParameter();
  if (value != ((0x20f3 + 61440) % 0x10000))
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  value = udvm.getMultitypeParameter();
  if (value != 8191)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 110nnnnn nnnnnnnn               memory[N]           0 - 65535
  value = udvm.getMultitypeParameter();
  if (value != 0x02c0)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 10000000 nnnnnnnn nnnnnnnn      N                   0 - 65535
  value = udvm.getMultitypeParameter();
  if (value != 0xbeef)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  // 10000001 nnnnnnnn nnnnnnnn      memory[N]           0 - 65535
  value = udvm.getMultitypeParameter();
  if (value != 0x8788)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  //==========================
  // Test Address Parameters
  //==========================
  udvm.m_pc = 2;
  value = udvm.getAddressParameter();
  if (value != 66)
  {
    std::cout << __FILE__ << ':' << __LINE__ << ": Bad decode" << std::endl;
    success = false;
  }

  return success;
}

bool test_osc_Udvm_opcode()
{
  std::cout << __FILE__ << ':' << __LINE__ 
    << ": Opcode tests not implemented"
    << std::endl;
  return false;
}


bool runOneTest(osc::Udvm &udvm, int i)
{
  bool status = true;
  osc::SigcompMessage *sm;

  static osc::TcpStream tcpStream;

  osc::byte_t output[0x10000];
  osc::BitBuffer outputBuffer(output,sizeof(output));

  std::cout << "    - [" << i << "] "<< tortureTest[i].name << std::endl;

  if (tortureTest[i].streamInput)
  {
    tcpStream.addData((osc::byte_t*)tortureTest[i].sigcompMessage,
                                 tortureTest[i].sigcompMessageLength);
    sm = tcpStream.getNextMessage();
  }
  else
  {
    sm = new osc::SigcompMessage((osc::byte_t*)tortureTest[i].sigcompMessage,
                                 tortureTest[i].sigcompMessageLength);
  }

  TEST_ASSERT(sm);

  if (!tortureTest[i].failure)
  {
    TEST_ASSERT(sm->isValid());
  }

  udvm.init(*sm, tortureTest[i].streamInput);

  if (sm->isValid())
  {
    udvm.setOutputBuffer(outputBuffer);
    udvm.execute();
  }

  // Make sure that successful tests don't cause UDVM failure
  if (!tortureTest[i].failure && udvm.isFailed())
  {
    std::cout << "      !!! UDVM failure (" << osc::s_nackCode[udvm.m_failureReason] 
              << ") at " << udvm.m_failureFile << ":" << udvm.m_failureLine
              << std::endl;
    status = false;
  }
  else if (tortureTest[i].failure != udvm.m_failureReason)
  {
    std::cout << "      !!! UDVM expected to fail (" 
              << osc::s_nackCode[tortureTest[i].failure]
              << "); actual status = " 
              << osc::s_nackCode[udvm.m_failureReason]
              << " at " << udvm.m_failureFile << ":" << udvm.m_failureLine
              << std::endl;
    status = false;
  }

  if (!tortureTest[i].failure)
  {
    // Compare the output to the expected output
    TEST_ASSERT_EQUAL(outputBuffer.getBufferSize()/8,
                      tortureTest[i].expectedOutputLength);

    TEST_ASSERT_EQUAL_BUFFERS(output, 
                              ((unsigned char *)tortureTest[i].expectedOutput),
                              tortureTest[i].expectedOutputLength);
  
    // Compare the cycles consumed to the expected number
    TEST_ASSERT_EQUAL(udvm.m_cyclesUsed, tortureTest[i].expectedCycles);

  }

  // Store state
  if (!udvm.isFailed())
  {
    osc::u32 id = tortureTest[i].compartmentId;
    osc::compartment_id_t cid(reinterpret_cast<osc::byte_t*>(&id), sizeof(id));

    osc::StateChanges *sc = udvm.getProposedStates();
    TEST_ASSERT(sc);
    udvm.m_stateHandler.processChanges(*sc, cid);
  }

  delete(sm);

  return status;
}


bool test_osc_Udvm_torture()
{
  int i;
  osc::StateHandler sh(2048, cyclesPerBit, 8192, 2);
  osc::Udvm udvm(sh, 16384);
  int numTotal = 0;
  int numSucceed = 0;
  bool flag;
  bool status = true;

  // Add the SIP dictionary
  osc::State *sipDict = new osc::SipDictionary();
  sh.addState(sipDict);

  for (i = 0; tortureTest[i].name; i++)
  {
    flag = runOneTest(udvm, i);
    status = status && flag;
    numTotal ++;
    numSucceed += (flag?1:0);
  }
  std::cout << "    Torture Tests: " << numSucceed << " of "
            << numTotal << " passed";
  std::cout << std::endl;
  return status;
}

/*
bool test_osc_Udvm()
{
  bool success = true;

  success = paramDecodeTest() && success;
  success = opcodeTest() && success;
  success = tortureTest() && success;

  return success;
}

static bool UdvmTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_Udvm,
                                     "test_osc_Udvm");
*/
}

static bool UdvmTestInitStatus = 
  osc::TestList::instance()->addTest(osc::test_osc_Udvm_paramDecode, 
                                     "test_osc_Udvm_paramDecode") &&
/*
  osc::TestList::instance()->addTest(osc::test_osc_Udvm_opcode, 
                                     "test_osc_Udvm_opcode") &&
*/
  osc::TestList::instance()->addTest(osc::test_osc_Udvm_torture, 
                                     "test_osc_Udvm_torture");
