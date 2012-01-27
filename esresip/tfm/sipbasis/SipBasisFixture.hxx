#ifndef _SipBasisFixture_hxx_
#define _SipBasisFixture_hxx_

#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"

#include "rutil/Data.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/TestUser.hxx"

namespace resip
{
class NameAddr;
class Data;
class SipStack;
class StackThread;
}

class TestUser;
class TestProxy;
class TestEndPoint;
class CommandLineParser;

class SipBasisDriver;

class SipBasisFixture : public CppUnit::TestFixture
{
   public:

      SipBasisFixture();
      
      virtual ~SipBasisFixture() ;
      virtual void setUp();
      virtual void tearDown();
      static void initialize(int argc, char** argv);
      static void destroyStatic();

      static TestUser* resip;
      static TestUser* resip2;

       static SipBasisDriver* sipbasis;
     
      
      
};

#endif

/* Copyright 2007 Estacado Systems */
