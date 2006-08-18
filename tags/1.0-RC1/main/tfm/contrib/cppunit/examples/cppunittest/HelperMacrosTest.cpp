#include "FailureException.h"
#include "HelperMacrosTest.h"
#include "HelperSuite.h"
#include "SubclassedTestCase.h"
#include <cppunit/TestResult.h>
#include <memory>

/* Note:
 - no unit test for CPPUNIT_TEST_SUITE_REGISTRATION...
 */

class FailTestCase : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE( FailTestCase );
  CPPUNIT_TEST_FAIL( testFail );
  CPPUNIT_TEST_SUITE_END();
public:
  void testFail()
  {
    CPPUNIT_ASSERT_MESSAGE( "Failure", false );
  }
};


class FailToFailTestCase : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE( FailToFailTestCase );
  CPPUNIT_TEST_FAIL( testFailToFail );
  CPPUNIT_TEST_SUITE_END();
public:
  void testFailToFail()
  {
  }
};


class ExceptionTestCase : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE( ExceptionTestCase );
  CPPUNIT_TEST_EXCEPTION( testException, FailureException );
  CPPUNIT_TEST_SUITE_END();
public:
  void testException()
  {
    throw FailureException();
  }
};


class ExceptionNotCaughtTestCase : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE( ExceptionNotCaughtTestCase );
  CPPUNIT_TEST_EXCEPTION( testExceptionNotCaught, FailureException );
  CPPUNIT_TEST_SUITE_END();
public:
  void testExceptionNotCaught()
  {
  }
};



CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( HelperMacrosTest, 
                                       CppUnitTest::helperSuiteName() );


HelperMacrosTest::HelperMacrosTest()
{
}


HelperMacrosTest::~HelperMacrosTest()
{
}


void 
HelperMacrosTest::setUp()
{
  m_testListener = new MockTestListener( "mock-testlistener" );
  m_result = new CppUnit::TestResult();
  m_result->addListener( m_testListener );
}


void 
HelperMacrosTest::tearDown()
{
  delete m_result;
  delete m_testListener;
}


void 
HelperMacrosTest::testNoSubclassing()
{
  std::auto_ptr<CppUnit::TestSuite> suite( BaseTestCase::suite() );
  CPPUNIT_ASSERT_EQUAL( 1, suite->countTestCases() );
  m_testListener->setExpectedStartTestCall( 1 );
  m_testListener->setExpectNoFailure();

  suite->run( m_result );
  m_testListener->verify();
}


void 
HelperMacrosTest::testSubclassing()
{
  std::auto_ptr<CppUnit::TestSuite> suite( SubclassedTestCase::suite() );
  CPPUNIT_ASSERT_EQUAL( 2, suite->countTestCases() );
  m_testListener->setExpectedStartTestCall( 2 );
  m_testListener->setExpectedAddFailureCall( 1 );

  suite->run( m_result );
  m_testListener->verify();
}


void 
HelperMacrosTest::testFail()
{
  std::auto_ptr<CppUnit::TestSuite> suite( FailTestCase::suite() );
  m_testListener->setExpectedStartTestCall( 1 );
  m_testListener->setExpectNoFailure();

  suite->run( m_result );
  m_testListener->verify();
}


void 
HelperMacrosTest::testFailToFail()
{
  std::auto_ptr<CppUnit::TestSuite> suite( FailToFailTestCase::suite() );
  m_testListener->setExpectedStartTestCall( 1 );
  m_testListener->setExpectedAddFailureCall( 1 );

  suite->run( m_result );
  m_testListener->verify();
}


void 
HelperMacrosTest::testException()
{
  std::auto_ptr<CppUnit::TestSuite> suite( ExceptionTestCase::suite() );
  m_testListener->setExpectedStartTestCall( 1 );
  m_testListener->setExpectNoFailure();
  
  suite->run( m_result );
  m_testListener->verify();
}


void 
HelperMacrosTest::testExceptionNotCaught()
{
  std::auto_ptr<CppUnit::TestSuite> suite( ExceptionNotCaughtTestCase::suite() );
  m_testListener->setExpectedStartTestCall( 1 );
  m_testListener->setExpectedAddFailureCall( 1 );

  suite->run( m_result );
  m_testListener->verify();
}
