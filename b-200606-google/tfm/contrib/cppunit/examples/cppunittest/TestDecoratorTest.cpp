#include "ExtensionSuite.h"
#include "TestDecoratorTest.h"
#include <cppunit/TestResult.h>


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestDecoratorTest,
                                       CppUnitTest::extensionSuiteName() );


TestDecoratorTest::TestDecoratorTest()
{
}


TestDecoratorTest::~TestDecoratorTest()
{
}


void 
TestDecoratorTest::setUp()
{
  m_test = new MockTestCase( "mocktest" );
  m_decorator = new CppUnit::TestDecorator( m_test );
}


void 
TestDecoratorTest::tearDown()
{
  delete m_decorator;
  delete m_test;
}


void 
TestDecoratorTest::testCountTestCases()
{
  m_test->setExpectedCountTestCasesCall( 1 );
  CPPUNIT_ASSERT_EQUAL( 1, m_decorator->countTestCases() );
  m_test->verify();
}


void 
TestDecoratorTest::testRun()
{
  m_test->setExpectedSetUpCall( 1 );
  m_test->setExpectedRunTestCall( 1 );
  m_test->setExpectedTearDownCall( 1 );
  CppUnit::TestResult result;

  m_decorator->run( &result );
  m_test->verify();
}


void 
TestDecoratorTest::testGetName()
{
  CPPUNIT_ASSERT_EQUAL( m_test->getName(), m_decorator->getName() );
}
