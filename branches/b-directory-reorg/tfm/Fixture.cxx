#include <cppunit/TestCase.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/TextTestResult.h>

#include "rutil/BaseException.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "tfm/CommandLineParser.hxx"
#include "tfm/DnsUtils.hxx"
#include "tfm/Fixture.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace std;
using resip::Data;
using resip::Uri;

TestProxy* Fixture::proxy = 0;
TestUser* Fixture::jason = 0;
TestUser* Fixture::jason1 = 0;
TestUser* Fixture::jason2 = 0;
TestUser* Fixture::jason3 = 0;
TestUser* Fixture::derek = 0;
TestUser* Fixture::david = 0;
TestUser* Fixture::enlai = 0;
TestUser* Fixture::cullen = 0;

Data Fixture::publicInterface;
Data Fixture::privateInterface;
resip::Uri Fixture::outboundProxy;

Fixture::Fixture() 
{
}

Fixture::~Fixture() 
{
}

static TestReproUser* makeReproUser(TestProxy& proxy, const Data& user, const Data& host)
{
   Uri j;
   j.user() = user;
   j.host() = host;
   return new TestReproUser(proxy, j, j.user(), j.user());
}

void
Fixture::initialize(int argc, char** argv)
{
#if 0
   proxy = new TestProxy("proxy",
                         "localhost", 
                         5060);
#else
   proxy = new TestRepro("proxy", "localhost", 5060);
   jason = makeReproUser(*proxy, "jason", "localhost");
   jason1 = makeReproUser(*proxy, "jason", "localhost");
   jason2 = makeReproUser(*proxy, "jason", "localhost");
   jason3 = makeReproUser(*proxy, "jason", "localhost");
   derek = makeReproUser(*proxy, "derek", "localhost");
   david = makeReproUser(*proxy, "david", "localhost");
   enlai = makeReproUser(*proxy, "enlai", "localhost");
   cullen = makeReproUser(*proxy, "cullen", "localhost");
#endif
}


void 
Fixture::setUp()
{
}

void 
Fixture::tearDown()
{
   DebugLog(<<"In teardown.");
   if (SequenceClass::CPUSequenceSet)
   {
      if (SequenceClass::CPUSequenceSet->executionFailed())
      {
         InfoLog(<<"===================================================");
         WarningLog(<<"FAILED: Sleeping until retransmissions finish.");
         InfoLog(<<"===================================================");
         sleep(35); // could sleep longer here
      }
      else
      {
         // !dlb! not quite right -- could have failed on CPPUNIT_ASSERT
         InfoLog(<<"===================================================");
         WarningLog(<<"PASSED");
         InfoLog(<<"===================================================");
      }
   }

   jason->clean();
   jason1->clean();
   jason2->clean();
   jason3->clean();
   derek->clean();
   david->clean();
   enlai->clean();
   cullen->clean();
}

      
void 
Fixture::destroyStatic()
{
   InfoLog(<< "Deleting proxy");
   delete proxy;
   proxy = 0;
}

#if 0
void
Fixture::makePserver(int argc, char** argv)
{
   char* proxyArg = 0;
   char* outboundArg = 0;
   char* pubInterfaceArg = 0;
   char* privateInterfaceArg = 0;

   struct poptOption table[200] = {
      {"proxy",    'x', POPT_ARG_STRING, &proxyArg,            0, "proxy:port",           ""},      
      {"private",  0,   POPT_ARG_STRING, &privateInterfaceArg, 0, "private interface IP", ""},
      {"public",   0,   POPT_ARG_STRING, &pubInterfaceArg,     0, "public interface IP",  ""},
      {"outbound", 0,   POPT_ARG_STRING, &outboundArg,         0, "specify outbound",     "sip:10.0.0.1:5000"},
      { NULL, 0, 0, NULL, 0 }
   };

   //Log::initialize(args.logType(), args.logLevel(), "test");
   
   if (pubInterfaceArg) publicInterface = pubInterfaceArg;
   if (privateInterfaceArg) privateInterface = privateInterfaceArg;
   
   
   if (!proxyArg)
   {
      InfoLog(<< "Creating local proxy.");
      proxy = new TestProxy("proxy",
                            (pubInterfaceArg ? pubInterfaceArg : resip::DnsUtil::getLocalHostName()), 
                            PortAllocator::getNextPort(),
                            (pubInterfaceArg ? pubInterfaceArg : Data::Empty));
      usleep(100000);
   }
   else
   {
      HostPort pr(proxyArg);
      
      if(pr.host() == "localhost")
      {
         InfoLog(<< "Creating local proxy on port " << pr.port());
         proxy = new PicassoTestProxy("proxy",
                                      (pubInterfaceArg ? pubInterfaceArg : resip::DnsUtil::getLocalHostName()), 
                                      pr.port(),
                                      (pubInterfaceArg ? pubInterfaceArg : Data::Empty),
                                      numThreadsArgs);
         usleep(100000);
         
      }
      else
      {
         proxy = new PicassoTestProxy("proxy", pr.host(), pr.port());
         proxy->addSources(DnsUtils::makeSourceSet(pr.host(), pr.port(), resip::UDP));
         InfoLog(<< "Using remote proxy " << proxy->getUri());
      }
   }

   if (outboundArg)
   {
      InfoLog(<< "Using outbound proxy " << outboundArg);
      outboundProxy = resip::Uri(outboundArg);
   }
}
#endif


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
