#include <cppunit/TextTestRunner.h>
#include <cppunit/TextTestResult.h>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "tfm/CommandLineParser.hxx"
#include "tfm/Fixture.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace resip;

static const int WaitFor100 = 1000;
static const int WaitFor180 = 1000;
static const int WaitFor487 = 1000;
static const int WaitForAck = 1000;  //immediate ACK for 4xx and CANCEL; not ACK for 200
static const int WaitForCommand = 1000;
static const int WaitForResponse = 1000;
static const int WaitForRegistration = 1000;
static const int PauseTime = 100;
static const int WaitForPause = 1100;
static const int WaitForEndOfTest = 1000;
static const int WaitForEndOfSeq = 1000;
static const int Seconds = 1000;

const Data transport("udp");
static NameAddr localhost;

class TestHolder : public Fixture
{
   public:
      
///***************************************** tests start here ********************************//
      void testRegisterBasic()
      {
         WarningLog(<<"*!testRegisterBasic!*");
         
         //TestUser jason(Uri("sip:jason@localhost"), "jason", "jason");
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      static boost::shared_ptr<SipMessage>
      largeCallId(boost::shared_ptr<SipMessage> msg)
      {
         const int oversize = 2048;
         Data callId(oversize, true);
         for (int i = 0; i < oversize/resip::Random::maxLength; ++i)
         {
            callId += resip::Random::getRandomHex(resip::Random::maxLength);
         }
         callId += resip::Random::getRandomHex(oversize - resip::Random::maxLength*(oversize/resip::Random::maxLength));
         msg->header(h_CallId).value() = callId;

         return msg;
      }

      void testOversizeCallIdRegister()
      {
         WarningLog(<<"*!testOversizeCallIdRegister!*");
         
         //TestUser jason(Uri("sip:jason@localhost"), "jason", "jason");
         Seq(condition(largeCallId, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/400, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testRegisterClientRetransmits()
      {
         WarningLog(<<"*!testRegisterClientRetransmits!*");

         //TestUser jason(Uri("sip:jason@localhost"), "jason", "jason");
         boost::shared_ptr<SipMessage> reg;
         Seq(save(reg, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->retransmit(reg)),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteClientRetransmissionsWithRecovery()
      {
         WarningLog(<<"*!testInviteClientRetransmissionsWithRecovery!*");
         //TestUser jason(Uri("sip:jason@localhost"), "jason", "jason");
         //TestUser derek(Uri("sip:derek@localhost"), "derek", "derek");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, derek->note("R1")),
                     jason->expect(INVITE, contact(derek), 1000, derek->note("R2")),
                     jason->expect(INVITE, contact(derek), 2000, chain(derek->note("R3"), jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor100, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testInviteBasic()
      {
         WarningLog(<<"*!testInviteBasic!*");
         //TestUser jason(Uri("sip:jason@localhost"), "jason", "jason");
         //TestUser derek(Uri("sip:derek@localhost"), "derek", "derek");
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(jason->invite(*derek),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                     jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
                     jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                     derek->expect(ACK, from(jason), WaitForResponse, jason->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }


      // provisioning here(automatic cleanup)
      static void createStatic()
      {
      }
};

#define TEST(_method) \
   suiteOfTests->addTest(new CppUnit::TestCaller<TestHolder>(#_method, &TestHolder::_method))
class MyTestCase
{
   public:
      static CppUnit::Test* suite()
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );
         TEST(testRegisterBasic);
         TEST(testRegisterClientRetransmits);
         TEST(testInviteBasic);
         TEST(testInviteClientRetransmissionsWithRecovery);
         //TEST(testOversizeCallIdRegister);
         return suiteOfTests;
      }
};

int main(int argc, char** argv)
{
   try
   {
      CommandLineParser args(argc, argv);
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
      resip::Timer::T100 = 0;
      
      TestHolder::createStatic();
      Fixture::initialize(argc, argv);
      
      CppUnit::TextUi::TestRunner runner;

      runner.addTest( MyTestCase::suite() );
      runner.run();
      DebugLog(<< "Finished");

      Fixture::destroyStatic();
   }
   catch (BaseException& e)
   {
      cerr << "Fatal error: " << e << endl;
      exit(-1);
   }

   return 0;
}

// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
