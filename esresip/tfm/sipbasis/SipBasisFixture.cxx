#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"
#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

#include "rutil/BaseException.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "tfm/CommandLineParser.hxx"
#include "tfm/DnsUtils.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"

#include "tfm/sipbasis/SipBasisFixture.hxx"
#include "tfm/sipbasis/SipBasisDriver.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace std;
using namespace resip;

TestUser* SipBasisFixture::resip = 0;
TestUser* SipBasisFixture::resip2 = 0;

SipBasisDriver* SipBasisFixture::sipbasis = 0;

SipBasisFixture::SipBasisFixture() 
{
}

SipBasisFixture::~SipBasisFixture() 
{
}

static TestUser* makeResipUser(const Data& user, const Data& host, Security* security)
{
   Uri j;
   j.user() = user;
   j.host() = host;
   return new TestUser(j, j.user(), j.user(), UDP, TestSipEndPoint::NoOutboundProxy, Data::Empty, security);

}

void
SipBasisFixture::initialize(int argc, char** argv)
{
//   stack = new SipStack;
//   stack->addTransport(UDP, 5060, V4);
//   stackThread = new StackThread(*stack);
//   stackThread->run();
   resip = makeResipUser("resip", "localhost", NULL);
   resip2 = makeResipUser("resip2", "localhost", NULL);
   sipbasis = new SipBasisDriver;
}


void 
SipBasisFixture::setUp()
{
}

void 
SipBasisFixture::tearDown()
{
   DebugLog(<<"In teardown.");
   if (SequenceClass::CPUSequenceSet)
   {
      if (SequenceClass::CPUSequenceSet->executionFailed())
      {
         InfoLog(<<"===================================================");
         WarningLog(<<"FAILED: Sleeping until retransmissions finish.");
         InfoLog(<<"===================================================");
#ifndef WIN32
         sleep(35); // could sleep longer here
#else
         Sleep(35000); // could sleep longer here
#endif
      }
      else
      {
         // !dlb! not quite right -- could have failed on CPPUNIT_ASSERT
         InfoLog(<<"===================================================");
         WarningLog(<<"PASSED");
         InfoLog(<<"===================================================");
      }
   }

   resip->clean();
   resip2->clean();
   //Not cleaning SipBasisDriver right now
}

      
void 
SipBasisFixture::destroyStatic()
{
// TODO : Is this where the stack gets cleaned up?
//   InfoLog(<< "Deleting proxy");
//   delete proxy;
//   proxy = 0;
}

/* Copyright 2007 Estacado Systems */
