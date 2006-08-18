#include "ExtensionSuite.h"
#include "OrthodoxTest.h"
#include <cppunit/extensions/Orthodox.h>
#include <cppunit/TestResult.h>

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( OrthodoxTest,
                                       CppUnitTest::extensionSuiteName() );

OrthodoxTest::OrthodoxTest()
{
}


OrthodoxTest::~OrthodoxTest()
{
}


void 
OrthodoxTest::setUp()
{
  m_testListener = new MockTestListener( "mock-listener" );
  m_result = new CppUnit::TestResult();
  m_result->addListener( m_testListener );
}


void 
OrthodoxTest::tearDown()
{
  delete m_result;
  delete m_testListener;
}


void 
OrthodoxTest::testValue()
{
  CppUnit::Orthodox<Value> test;
  m_testListener->setExpectNoFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadConstructor()
{
  CppUnit::Orthodox<ValueBadConstructor> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadInvert()
{
  CppUnit::Orthodox<ValueBadInvert> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadEqual()
{
  CppUnit::Orthodox<ValueBadEqual> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadNotEqual()
{
  CppUnit::Orthodox<ValueBadNotEqual> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadCall()
{
  CppUnit::Orthodox<ValueBadCall> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}


void 
OrthodoxTest::testValueBadAssignment()
{
  CppUnit::Orthodox<ValueBadAssignment> test;
  m_testListener->setExpectFailure();
  test.run( m_result );
  m_testListener->verify();
}
