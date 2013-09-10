#ifndef _DragonFixture_hxx_
#define _DragonFixture_hxx_

#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"

#include "rutil/Data.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/repro/CommandLineParser.hxx"

namespace resip
{
class NameAddr;
class Data;
}

class TestUser;
class TestProxy;
class TestEndPoint;
class CommandLineParser;

class ReproFixture : public CppUnit::TestFixture
{
   public:

      ReproFixture();
      
      virtual ~ReproFixture() ;
      virtual void setUp();
      virtual void tearDown();
      static void initialize(CommandLineParser& args);
      static void destroyStatic();

      static resip::Security* security;
      static TestProxy* proxy;
      static TestUser* jason;
      static TestUser* jason1;
      static TestUser* jason2;
      static TestUser* jason3;
      static TestUser* derek;
      static TestUser* david;
      static TestUser* enlai;
      static TestUser* cullen;
      static TestUser* jozsef;
      static TestUser* jasonTcp;
      static TestUser* derekTcp;
      static TestUser* davidTcp;
      static TestUser* enlaiTcp;
      static TestUser* cullenTcp;
      static TestUser* jasonTls;
      static TestUser* derekTls;
      static TestUser* davidTls;
      static TestUser* enlaiTls;
      static TestUser* cullenTls;
      
      //static TestUser jason;
      static resip::Data publicInterface;
      static resip::Data privateInterface;
      static resip::Uri outboundProxy;

      
};

#endif

// Copyright 2004 Purplecomm, Inc.
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
