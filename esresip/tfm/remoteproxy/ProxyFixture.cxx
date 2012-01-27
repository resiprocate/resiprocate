
#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"
#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"


#include "rutil/BaseException.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "tfm/remoteproxy/CommandLineParser.hxx"
#include "tfm/DnsUtils.hxx"
#include "tfm/remoteproxy/ProxyFixture.hxx"
#include "tfm/remoteproxy/TestRemoteProxy.hxx"
#include "tfm/remoteproxy/TestProxyUser.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace std;
using namespace resip;

Security* ProxyFixture::security = 0;
TestProxy* ProxyFixture::proxy = 0;
TestUser* ProxyFixture::jason = 0;
TestUser* ProxyFixture::jason1 = 0;
TestUser* ProxyFixture::jason2 = 0;
TestUser* ProxyFixture::jason3 = 0;
TestUser* ProxyFixture::derek = 0;
TestUser* ProxyFixture::david = 0;
TestUser* ProxyFixture::enlai = 0;
TestUser* ProxyFixture::cullen = 0;
TestUser* ProxyFixture::jasonTcp = 0;
TestUser* ProxyFixture::derekTcp = 0;
TestUser* ProxyFixture::davidTcp = 0;
TestUser* ProxyFixture::enlaiTcp = 0;
TestUser* ProxyFixture::cullenTcp = 0;
TestUser* ProxyFixture::jasonTls = 0;
TestUser* ProxyFixture::derekTls = 0;
TestUser* ProxyFixture::davidTls = 0;
TestUser* ProxyFixture::enlaiTls = 0;
TestUser* ProxyFixture::cullenTls = 0;
TestUser* ProxyFixture::robert = 0;
TestUser* ProxyFixture::adam = 0;
TestUser* ProxyFixture::ron = 0;
TestUser* ProxyFixture::ben = 0;
TestUser* ProxyFixture::ajay = 0;
TestUser* ProxyFixture::voicemail=0;

Data ProxyFixture::publicInterface;
Data ProxyFixture::privateInterface;
resip::Uri ProxyFixture::outboundProxy;

ProxyFixture::ProxyFixture() 
{
}

ProxyFixture::~ProxyFixture() 
{
}

static TestProxyUser* makeProxyUser(TestProxy& proxy, const Data& user, const Data& host, const TransportType trans, const Data& nwIf, Security* security)
{
   Uri j;
   j.user() = user;
   j.host() = host;
   
   
   return new TestProxyUser(proxy, j, j.user(), j.user(),trans, nwIf, security);

}

void
ProxyFixture::initialize(int argc, char** argv)
{
#if 0
   proxy = new TestProxy("proxy",
                         args.mProxyHostName, 
                         5060);
#else
   // enable for TLS testing
   security = new resip::Security(getenv("PWD"));
   CommandLineParser args(argc,argv);
   initialize(args);
#endif
}

void
ProxyFixture::initialize(const CommandLineParser& args)
{
   if(security)
   {
      security->preload();
   }
   proxy = new TestRemoteProxy("proxy", args.mProxyHostName, args.mUdpPorts, args.mTcpPorts, args.mTlsPorts, args.mDtlsPorts, Data::Empty);
   jason = makeProxyUser(*proxy, "jason", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   jason1 = makeProxyUser(*proxy, "jason", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   jason2 = makeProxyUser(*proxy, "jason", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   jason3 = makeProxyUser(*proxy, "jason", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   derek = makeProxyUser(*proxy, "derek", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   david = makeProxyUser(*proxy, "david", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   enlai = makeProxyUser(*proxy, "enlai", args.mProxyHostName,UDP, args.mUserIPAddr,security);
   cullen = makeProxyUser(*proxy, "cullen", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   jasonTcp=makeProxyUser(*proxy,"jason",args.mProxyHostName,TCP,args.mUserIPAddr,security);
   derekTcp = makeProxyUser(*proxy, "derek", args.mProxyHostName,TCP,args.mUserIPAddr, security);
   davidTcp = makeProxyUser(*proxy, "david", args.mProxyHostName,TCP,args.mUserIPAddr, security);
   enlaiTcp = makeProxyUser(*proxy, "enlai", args.mProxyHostName,TCP,args.mUserIPAddr, security);
   cullenTcp = makeProxyUser(*proxy, "cullen", args.mProxyHostName,TCP,args.mUserIPAddr, security);
   jasonTls=makeProxyUser(*proxy,"jason",args.mProxyHostName,TLS,args.mUserIPAddr,security);
   derekTls = makeProxyUser(*proxy, "derek", args.mProxyHostName,TLS,args.mUserIPAddr, security);
   davidTls = makeProxyUser(*proxy, "david", args.mProxyHostName,TLS,args.mUserIPAddr, security);
   enlaiTls = makeProxyUser(*proxy, "enlai", args.mProxyHostName,TLS,args.mUserIPAddr, security);
   cullenTls = makeProxyUser(*proxy, "cullen", args.mProxyHostName,TLS, args.mUserIPAddr,security);
   robert = makeProxyUser(*proxy, "robert", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   adam = makeProxyUser(*proxy, "adam", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   ron = makeProxyUser(*proxy, "ron", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   ben = makeProxyUser(*proxy, "ben", args.mProxyHostName,UDP, args.mUserIPAddr,security);
   ajay = makeProxyUser(*proxy, "ajay", args.mProxyHostName,UDP,args.mUserIPAddr, security);
   voicemail=makeProxyUser(*proxy,"voicemail",args.mProxyHostName,UDP,args.mUserIPAddr,security);
}


void 
ProxyFixture::setUp()
{
}

void 
ProxyFixture::tearDown()
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

   jason->clean();
   jason1->clean();
   jason2->clean();
   jason3->clean();
   derek->clean();
   david->clean();
   enlai->clean();
   cullen->clean();
   jasonTcp->clean();
   derekTcp->clean();
   davidTcp->clean();
   enlaiTcp->clean();
   cullenTcp->clean();
   jasonTls->clean();
   derekTls->clean();
   davidTls->clean();
   enlaiTls->clean();
   cullenTls->clean();
   
}

      
void 
ProxyFixture::destroyStatic()
{
   delete jason;
   delete jason1;
   delete jason2;
   delete jason3;
   delete derek;
   delete david;
   delete enlai;
   delete cullen;
   delete jasonTcp;
   delete derekTcp;
   delete davidTcp;
   delete enlaiTcp;
   delete cullenTcp;
   delete jasonTls;
   delete derekTls;
   delete davidTls;
   delete enlaiTls;
   delete cullenTls;
   InfoLog(<< "Deleting proxy");
   delete proxy;
   proxy = 0;
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
