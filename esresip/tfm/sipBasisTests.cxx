#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "tfm/CommandLineParser.hxx"
#include "tfm/RouteGuard.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/predicates/ExpectUtils.hxx"

#include "tfm/sipbasis/SipBasisFixture.hxx"

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

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

class TestHolder : public SipBasisFixture
{
   public:
      
///***************************************** tests start here ********************************//

      void testTFMSanity()
      {
         WarningLog(<<"*!testTFMSanity!*");
         
         Seq(resip->invite((resip2->getContact()).uri()),
             And(Sub(optional(resip->expect(INVITE/100, from(resip2), WaitFor100, resip->noAction()))),
                 Sub(resip2->expect(INVITE, contact(resip), WaitForCommand, chain(resip2->ring(), resip2->answer())),
                     resip->expect(INVITE/180, from(resip2), WaitFor100, resip->noAction()),
                     resip->expect(INVITE/200, contact(resip2), WaitForResponse, resip->ack()),
                     resip2->expect(ACK, from(resip), WaitForResponse, resip->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }


      void testBasicMessage()
      {
         WarningLog(<<"*!testBasicMessage!*");

         resip::NameAddr target("<sip:sipbasis@localhost:6060>");

         Seq(resip->message(target,"respond: 200"),
             resip->expect(MESSAGE/200, from(target), WaitForResponse, resip->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testMessageClientRetransmits()
      {
         WarningLog(<<"*!testMessageClientRetransmits!*");

         resip::NameAddr target("<sip:sipbasis@localhost:6060>");
         boost::shared_ptr<SipMessage> msg1;
         boost::shared_ptr<SipMessage> msg2;
         Seq(save(msg1,resip->message(target,"respond: 200")),
             resip->expect(MESSAGE/200, unknownHeaderMatch("X-magicAvalue","1"), WaitForResponse, save(msg2,resip->message(target,"respond: 200"))),
             resip->expect(MESSAGE/200, unknownHeaderMatch("X-magicAvalue","2"), WaitForResponse, resip->retransmit(msg1)),
             resip->expect(MESSAGE/200, unknownHeaderMatch("X-magicAvalue","1"), WaitForResponse, resip->retransmit(msg2)),
             resip->expect(MESSAGE/200, unknownHeaderMatch("X-magicAvalue","2"), WaitForResponse, resip->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      static void createStatic() {};

};

#define TEST(_method) \
   suiteOfTests->addTest(new CppUnit::TestCaller<TestHolder>(#_method, &TestHolder::_method))
class MyTestCase
{
   public:
      static CppUnit::Test* suite()
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );
         
         TEST(testTFMSanity);
         TEST(testBasicMessage);
         TEST(testMessageClientRetransmits);

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
      SipBasisFixture::initialize(argc, argv);
      
      CppUnit::TextUi::TestRunner runner;

      runner.addTest( MyTestCase::suite() );
      runner.run();
      DebugLog(<< "Finished");

      SipBasisFixture::destroyStatic();
   }
   catch (BaseException& e)
   {
      cerr << "Fatal error: " << e << endl;
      exit(-1);
   }

   return 0;
}

/* Copyright 2007 Estacado Systems */

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
