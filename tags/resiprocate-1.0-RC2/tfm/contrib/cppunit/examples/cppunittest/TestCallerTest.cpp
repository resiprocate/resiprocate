#include "FailureException.h"
#include "HelperSuite.h"
#include "TestCallerTest.h"
#include <cppunit/extensions/TestSuiteBuilder.h>
#include <cppunit/extensions/HelperMacros.h>


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestCallerTest, 
                                       CppUnitTest::helperSuiteName() );


void 
TestCallerTest::ExceptionThrower::testThrowFailureException()
{
  throw FailureException();
}


void 
TestCallerTest::ExceptionThrower::testThrowException()
{
  throw CppUnit::Exception( "expected Exception" );
}


void 
TestCallerTest::ExceptionThrower::testThrowNothing()
{
}



TestCallerTest::TestCallerTest() : 
    m_testName( "TrackedTestCaseCaller" )
{
}


TestCallerTest::~TestCallerTest()
{
}


void 
TestCallerTest::setUp()
{
  m_constructorCount = 0;
  m_destructorCount = 0;
  m_setUpCount = 0;
  m_tearDownCount = 0;
  m_testCount = 0;
  TrackedTestCase::setTracker( this );
  m_testListener = new MockTestListener( "listener1" );
  m_result = new CppUnit::TestResult();
  m_result->addListener( m_testListener );
}


void 
TestCallerTest::tearDown()
{
  TrackedTestCase::removeTracker();
  delete m_result;
  delete m_testListener;
}


void 
TestCallerTest::onConstructor()
{
  m_constructorCount++;
}


void 
TestCallerTest::onDestructor()
{
  m_destructorCount++;
}


void 
TestCallerTest::onSetUp()
{
  m_setUpCount++;
}


void 
TestCallerTest::onTearDown()
{
  m_tearDownCount++;
}


void 
TestCallerTest::onTest()
{
  m_testCount++;
}


void 
TestCallerTest::testBasicConstructor()
{
  {
    CppUnit::TestCaller<TrackedTestCase> caller( m_testName, 
                                                 &TrackedTestCase::test );
    checkTestName( caller.getName() );
    checkNothingButConstructorCalled();

    caller.run( m_result );

    checkRunningSequenceCalled();
  } // Force destruction of the test caller.
  CPPUNIT_ASSERT_EQUAL( 1, m_destructorCount );
}


void 
TestCallerTest::testReferenceConstructor()
{
  TrackedTestCase testCase;
  {
    CppUnit::TestCaller<TrackedTestCase> caller( "TrackedTestCaseCaller", 
                                                 &TrackedTestCase::test, 
                                                 testCase );
    checkTestName( caller.getName() );
    checkNothingButConstructorCalled();

    caller.run( m_result );

    checkRunningSequenceCalled();
  } // Force destruction of the test caller.
  CPPUNIT_ASSERT_EQUAL( 0, m_destructorCount );
}


void 
TestCallerTest::testPointerConstructor()
{
  TrackedTestCase *testCase = new TrackedTestCase();
  {
    CppUnit::TestCaller<TrackedTestCase> caller( m_testName, 
                                                 &TrackedTestCase::test, 
                                                 testCase );
    checkTestName( caller.getName() );
    checkNothingButConstructorCalled();

    caller.run( m_result );

    checkRunningSequenceCalled();
  } // Force destruction of the test caller.
  CPPUNIT_ASSERT_EQUAL( 1, m_destructorCount );
}


void 
TestCallerTest::testExpectFailureException()
{
  CppUnit::TestCaller<ExceptionThrower,FailureException> caller( 
      m_testName,
      &ExceptionThrower::testThrowFailureException );
  m_testListener->setExpectNoFailure();
  caller.run( m_result );
  m_testListener->verify();
}


void 
TestCallerTest::testExpectException()
{
  CppUnit::TestCaller<ExceptionThrower,CppUnit::Exception> caller( 
      m_testName,
      &ExceptionThrower::testThrowException );
  m_testListener->setExpectNoFailure();
  caller.run( m_result );
  m_testListener->verify();
}


void 
TestCallerTest::testExpectedExceptionNotCaught()
{
  CppUnit::TestCaller<ExceptionThrower,FailureException> caller( 
      m_testName,
      &ExceptionThrower::testThrowNothing );
  m_testListener->setExpectedAddFailureCall( 1 );
  caller.run( m_result );
  m_testListener->verify();
}


void 
TestCallerTest::checkNothingButConstructorCalled()
{
  CPPUNIT_ASSERT_EQUAL( 1, m_constructorCount );
  CPPUNIT_ASSERT_EQUAL( 0, m_destructorCount );
  CPPUNIT_ASSERT_EQUAL( 0, m_setUpCount );
  CPPUNIT_ASSERT_EQUAL( 0, m_tearDownCount );
  CPPUNIT_ASSERT_EQUAL( 0, m_testCount );
}


void 
TestCallerTest::checkRunningSequenceCalled()
{
  CPPUNIT_ASSERT_EQUAL( 1, m_setUpCount );
  CPPUNIT_ASSERT_EQUAL( 1, m_testCount );
  CPPUNIT_ASSERT_EQUAL( 1, m_tearDownCount );
}


void 
TestCallerTest::checkTestName( std::string testName )
{
  CPPUNIT_ASSERT( testName == m_testName );
}
