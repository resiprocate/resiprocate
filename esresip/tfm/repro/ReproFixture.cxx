#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"
#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

#include "rutil/BaseException.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "tfm/repro/CommandLineParser.hxx"
#include "tfm/DnsUtils.hxx"
#include "tfm/repro/ReproFixture.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace std;
using namespace resip;

Security* ReproFixture::security = 0;
TestProxy* ReproFixture::proxy = 0;
TestUser* ReproFixture::jason = 0;
TestUser* ReproFixture::jason1 = 0;
TestUser* ReproFixture::jason2 = 0;
TestUser* ReproFixture::jason3 = 0;
TestUser* ReproFixture::derek = 0;
TestUser* ReproFixture::david = 0;
TestUser* ReproFixture::enlai = 0;
TestUser* ReproFixture::cullen = 0;
TestUser* ReproFixture::jozsef = 0;
TestUser* ReproFixture::jasonTcp = 0;
TestUser* ReproFixture::derekTcp = 0;
TestUser* ReproFixture::davidTcp = 0;
TestUser* ReproFixture::enlaiTcp = 0;
TestUser* ReproFixture::cullenTcp = 0;
TestUser* ReproFixture::jasonSctp = 0;
TestUser* ReproFixture::derekSctp = 0;
TestUser* ReproFixture::davidSctp = 0;
TestUser* ReproFixture::enlaiSctp = 0;
TestUser* ReproFixture::cullenSctp = 0;
TestUser* ReproFixture::jasonTls = 0;
TestUser* ReproFixture::derekTls = 0;
TestUser* ReproFixture::davidTls = 0;
TestUser* ReproFixture::enlaiTls = 0;
TestUser* ReproFixture::cullenTls = 0;
TestUser* ReproFixture::robert = 0;
TestUser* ReproFixture::adam = 0;
TestUser* ReproFixture::ron = 0;
TestUser* ReproFixture::ben = 0;
TestUser* ReproFixture::ajay = 0;
TestUser* ReproFixture::voicemail=0;

Data ReproFixture::publicInterface;
Data ReproFixture::privateInterface;
resip::Uri ReproFixture::outboundProxy;

ReproFixture::ReproFixture() 
{
}

ReproFixture::~ReproFixture() 
{
}

static TestReproUser* makeReproUser(TestProxy& proxy, const Data& user, const Data& host, bool provideFailover, const TransportType trans, Security* security)
{
   Uri j;
   j.user() = user;
   j.host() = host;
   
   
   return new TestReproUser(proxy, j, j.user(), j.user(), provideFailover,trans, TestSipEndPoint::NoOutboundProxy, Data::Empty, security);

}

void
ReproFixture::initialize(int argc, char** argv)
{
#if 0
   proxy = new TestProxy("proxy",
                         "localhost", 
                         5060);
#else
   // enable for TLS testing
   security = new resip::Security(getenv("PWD"));
   CommandLineParser args(argc,argv);
   initialize(args);
#endif
}

void
ReproFixture::initialize(const CommandLineParser& args)
{
   proxy = new TestRepro("proxy", "localhost", args, "127.0.0.1", security);
   jason = makeReproUser(*proxy, "jason", "localhost",false,UDP, security);
   jason1 = makeReproUser(*proxy, "jason", "localhost",false,UDP, security);
   jason2 = makeReproUser(*proxy, "jason", "localhost",false,UDP, security);
   jason3 = makeReproUser(*proxy, "jason", "localhost",false,UDP, security);
   derek = makeReproUser(*proxy, "derek", "localhost",false,UDP, security);
   david = makeReproUser(*proxy, "david", "localhost",false,UDP, security);
   enlai = makeReproUser(*proxy, "enlai", "localhost",false,UDP, security);
   cullen = makeReproUser(*proxy, "cullen", "localhost",false,UDP, security);
   jozsef = makeReproUser(*proxy, "jozsef", "localhost", false, TCP, security);
   jasonTcp=makeReproUser(*proxy,"jason","localhost",false,TCP,security);
   derekTcp = makeReproUser(*proxy, "derek", "localhost",false,TCP, security);
   davidTcp = makeReproUser(*proxy, "david", "localhost",false,TCP, security);
   enlaiTcp = makeReproUser(*proxy, "enlai", "localhost",false,TCP, security);
   cullenTcp = makeReproUser(*proxy, "cullen", "localhost",false,TCP, security);

#ifdef RESIP_USE_SCTP
   jasonSctp=makeReproUser(*proxy,"jason","localhost",false,SCTP,security);
   derekSctp = makeReproUser(*proxy, "derek", "localhost",false,SCTP, security);
   davidSctp = makeReproUser(*proxy, "david", "localhost",false,SCTP, security);
   enlaiSctp = makeReproUser(*proxy, "enlai", "localhost",false,SCTP, security);
   cullenSctp = makeReproUser(*proxy, "cullen", "localhost",false,SCTP, security);
#endif

   jasonTls=makeReproUser(*proxy,"jason","localhost",false,TLS,security);
   derekTls = makeReproUser(*proxy, "derek", "localhost",false,TLS, security);
   davidTls = makeReproUser(*proxy, "david", "localhost",false,TLS, security);
   enlaiTls = makeReproUser(*proxy, "enlai", "localhost",false,TLS, security);
   cullenTls = makeReproUser(*proxy, "cullen", "localhost",false,TLS, security);
   robert = makeReproUser(*proxy, "robert", "localhost",true,UDP, security);
   adam = makeReproUser(*proxy, "adam", "localhost",true,UDP, security);
   ron = makeReproUser(*proxy, "ron", "localhost",true,UDP, security);
   ben = makeReproUser(*proxy, "ben", "localhost",true,UDP, security);
   ajay = makeReproUser(*proxy, "ajay", "localhost",true,UDP, security);
   voicemail=makeReproUser(*proxy,"voicemail","localhost",false,UDP,security);
}


void 
ReproFixture::setUp()
{
}

void 
ReproFixture::tearDown()
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
         sleep(65); // could sleep longer here
#else
         Sleep(65000); // could sleep longer here
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

   jason->clean();
   jason1->clean();
   jason2->clean();
   jason3->clean();
   derek->clean();
   david->clean();
   enlai->clean();
   cullen->clean();
   jozsef->clean();
   jasonTcp->clean();
   derekTcp->clean();
   davidTcp->clean();
   enlaiTcp->clean();
   cullenTcp->clean();
#ifdef RESIP_USE_SCTP
   jasonSctp->clean();
   derekSctp->clean();
   davidSctp->clean();
   enlaiSctp->clean();
   cullenSctp->clean();
#endif
   jasonTls->clean();
   derekTls->clean();
   davidTls->clean();
   enlaiTls->clean();
   cullenTls->clean();
   
}

      
void 
ReproFixture::destroyStatic()
{
   delete jason;
   delete jason1;
   delete jason2;
   delete jason3;
   delete derek;
   delete david;
   delete enlai;
   delete cullen;
   delete jozsef;
   delete jasonTcp;
   delete derekTcp;
   delete davidTcp;
   delete enlaiTcp;
   delete cullenTcp;
   delete jasonSctp;
   delete derekSctp;
   delete davidSctp;
   delete enlaiSctp;
   delete cullenSctp;
   delete jasonTls;
   delete derekTls;
   delete davidTls;
   delete enlaiTls;
   delete cullenTls;
   InfoLog(<< "Deleting proxy");
   delete proxy;
   proxy = 0;
}

#if 0
void
ReproFixture::makePserver(int argc, char** argv)
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
