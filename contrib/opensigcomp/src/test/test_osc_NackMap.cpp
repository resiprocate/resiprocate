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
#include "StateHandler.h"
#include "SigcompMessage.h"
#include "TestList.h"
#include "osc_generators.h"
#include "NackCodes.h"

osc::byte_t nackHead[]=
{
  0xF8, // 1 Byte header
  0x0,0x0| //Code Length
  0x1, //Version
  osc::SEGFAULT,//Failure code
  0x30,//Opcode
  0x10,0x00 //PC @ failure
//01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20
//00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
};

osc::SigcompMessage * createNackMessage(osc::sha1_t &sha1)
{

  osc::byte_t * temp = new osc::byte_t[sizeof(nackHead)+20];
  memmove(temp,nackHead,sizeof(nackHead));
  memmove(temp+sizeof(nackHead),sha1.digest,20);
  osc::SigcompMessage * message = 
    new osc::SigcompMessage( temp, sizeof(nackHead)+20 );
  message->takeOwnership(temp);
  return message;
}

bool test_osc_NackMap(){
  osc::sha1_t testNackSha1s[] = 
  {
    { { 197, 144,  10,  58, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 178,  41,  58,  77, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 76,   10, 197, 179, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 75,   58, 245, 180, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 241,  77,  41,  14, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 56,  179,  77, 199, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 87,  180, 178, 168, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 144,  14, 214, 111, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 41,  199, 144, 214, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 10,  168, 179, 245, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 58,  111,  76, 197, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 77,  214, 111, 178, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 179, 245,  87,  76, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 180, 197, 180,  75, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 14,  178,  75, 241, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 199,  76, 168,  56, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 168,  75, 56,   87, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 111, 241, 14,  144, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 214,  56, 241,  41, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } },
    { { 245,  87, 199,  10, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } }
  };
  
  osc::SigcompMessage ** nackMessages = new osc::SigcompMessage *[20];
  for(unsigned int y = 0; y < 20; y++)
  {
    nackMessages[y] = createNackMessage(testNackSha1s[y]);
  }
  bool test = true;

  osc::byte_t nackMessage1[] =
  {
    0xF8, // 1 Byte header
    0x0,0x0| //Code Length
    0x1, //Version
    osc::SEGFAULT,//Failure code
    0x30,//Opcode
    0x10,0x00, //PC @ failure
    1,65,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0 //SHA-1
  };
  //osc::sha1_t nackSha11 = { {1,65,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0} };
  osc::byte_t nackMessage2[] =
  {
    0xF8, // 1 Byte header
    0x0,0x0| //Code Length
    0x1, //Version
    osc::SEGFAULT,//Failure code
    0x30,//Opcode
    0x10,0x00, //PC @ failure
    2,162,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,1 //SHA-1 
  };
  //osc::sha1_t nackSha12 = { {2,162,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,1} };
  osc::byte_t nackMessage3[] =
  {
    0xF8, // 1 Byte header
    0x0,0x0| //Code Length
    0x1, //Version
    osc::SEGFAULT,//Failure code
    0x30,//Opcode
    0x10,0x00, //PC @ failure
    3,100,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,2 //SHA-1
  };
  //osc::sha1_t nackSha13 = { {3,100,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,2} };
  osc::byte_t nackMessage4[] =
  {
    0xF8, // 1 Byte header
    0x0,0x0| //Code Length
    0x1, //Version
    osc::SEGFAULT,//Failure code
    0x30,//Opcode
    0x10,0x00, //PC @ failure
    4,200,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,3 //SHA-1 
  };
  //osc::sha1_t nackSha14 = { {4,200,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,3} };
  osc::byte_t nackMessage5[] =
  {
    0xF8, // 1 Byte header
    0x0,0x0| //Code Length
    0x1, //Version
    osc::SEGFAULT,//Failure code
    0x30,//Opcode
    0x10,0x00, //PC @ failure
    5,112,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,4 //SHA-1 
  };
  //osc::sha1_t nackSha15 = { {5,112,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,4} };
  osc::StateHandler * handler = new osc::StateHandler();
  osc::Compartment * compartment1 = handler->getCompartment( osc::generateRandomCompartmentId(4) );
  osc::Compartment * compartment2 = handler->getCompartment( osc::generateRandomCompartmentId(4) );
  osc::SigcompMessage message1(nackMessage1,sizeof(nackMessage1));
  assert(message1.isValid());
  assert(message1.isNack());
  osc::SigcompMessage message2(nackMessage2,sizeof(nackMessage2));
  assert(message2.isValid());
  assert(message2.isNack());
  osc::SigcompMessage message3(nackMessage3,sizeof(nackMessage3));
  assert(message3.isValid());
  assert(message3.isNack());
  osc::SigcompMessage message4(nackMessage4,sizeof(nackMessage4));
  assert(message4.isValid());
  assert(message4.isNack());
  osc::SigcompMessage message5(nackMessage5,sizeof(nackMessage5));
  assert(message5.isValid());
  assert(message5.isNack());
  ///////////// Adding a NACK to Compartment Association ////////////////////
  handler->addNack(message1,compartment1);
  test = test && osc::print_subtest("Single NACK single Compartment association ", 1 == handler->numberOfNacks());
  //////////// Adding multiple NACK to Compartment Associations /////////////
  handler->addNack(message2,compartment2);
  test = test && osc::print_subtest("Multiple single NACK to single Compartment associations ", 2 == handler->numberOfNacks());
  delete handler;
  //////////// Adding a NACK multiple times to a Compartment /////////////////
  //@tdod Test NackMap maximum message count
  handler = new osc::StateHandler(8192,64,40*1024,1,4);
  osc::Compartment * compartment3 = handler->getCompartment( osc::generateRandomCompartmentId(4) );
  handler->addNack(message1,compartment3);
  handler->addNack(message1,compartment3);

  // Collisions should be discarded. There is no value in having
  // multiple pointers from the same NACK to the same compartment
  // (or, really, even to different compartments).

  test = test && osc::print_subtest("Single NACK multiple times to single Compartment associations ", 1 == handler->numberOfNacks());

  /////////// Nack Size Limit (Base condition) //////////////////////////////
  handler->addNack(message1,compartment3);
  handler->addNack(message2,compartment3);
  handler->addNack(message3,compartment3);
  handler->addNack(message4,compartment3);
  handler->addNack(message5,compartment3);
  test = test && osc::print_subtest("NackMap size limit ", 4 == handler->numberOfNacks());
  delete handler;
  /////////// NACK Find ///////////////////////////////////////////////////
  handler = new osc::StateHandler(8192,64,40*1024,1,4);
  osc::Compartment * compartment4 = handler->getCompartment( osc::generateRandomCompartmentId(4) );
  compartment3 = handler->getCompartment( osc::generateRandomCompartmentId(3) );
  for(int x = 0; x < 20; x++)
  {
    handler->addNack(*nackMessages[x],compartment4);
  }
  handler->addNack(message1,compartment3);
  osc::sha1_t sha1;
  nackMessages[19]->getSha1Hash(sha1.digest,20);
  bool expTest = (0 != (handler->findNackedCompartment(sha1)));
  test = test && osc::print_subtest("NackMap nack lookup", expTest);
  delete handler;

  ///////////// Removing Nacked Stale Compartment ////////////////////////////
  handler = new osc::StateHandler(8192,64,40*1024,1,4);
  osc::Compartment * compartment5 = handler->getCompartment
        ( osc::generateRandomCompartmentId(4) );
  handler->addNack(*nackMessages[0],compartment5);
  compartment5->release();
  handler->removeStaleCompartments();
  handler->removeStaleCompartments();
  test = test && osc::print_subtest("Removing nacked stale compartment ",
    0==handler->numberOfCompartments());
  delete handler;

  ///////////// Removing Singularly nacked stale compartment //////////////////
  handler = new osc::StateHandler(8192,64,40*1024,1,4);
  osc::Compartment * compartment6 = handler->getCompartment
        ( osc::generateRandomCompartmentId(4) );
  for(int z = 0; z < 10; z++)
  {
    handler->addNack(*nackMessages[1],compartment6);
  }
  compartment6->release();
  handler->removeStaleCompartments();
  handler->removeStaleCompartments();
  test = test && osc::print_subtest("Removing once nacked stale compartment ",
    0==handler->numberOfCompartments());
  delete handler;

  for(unsigned int w = 0; w < 20; w++)
  {
    delete nackMessages[w];
  }
  delete [] nackMessages;

  return test;
}

static bool NackMapTestInitStatus = osc::TestList::instance()->addTest
  (test_osc_NackMap, "test_osc_NackMap");
  


