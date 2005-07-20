#include <cppunit/TestAssert.h>
#include <cppunit/TestFailure.h>
#include "MockTestListener.h"


MockTestListener::MockTestListener( std::string name )
    : m_name( name )
    , m_hasExpectationForStartTest( false )
    , m_hasParametersExpectationForStartTest( false )
    , m_expectedStartTestCallCount( 0 )
    , m_startTestCall( 0 )
    , m_hasExpectationForEndTest( false )
    , m_hasParametersExpectationForEndTest( false )
    , m_expectedEndTestCallCount( 0 )
    , m_endTestCall( 0 )
    , m_hasExpectationForAddFailure( false )
    , m_hasExpectationForSomeFailure( false )
    , m_hasParametersExpectationForAddFailure( false )
    , m_expectedAddFailureCallCount( 0 )
    , m_addFailureCall( 0 )
    , m_expectedFailedTest( NULL )
    , m_expectedException( NULL )
    , m_expectedIsError( false )
{
}


void 
MockTestListener::setExpectFailure( CppUnit::Test *failedTest,
                                    CppUnit::Exception *thrownException,
                                    bool isError )
{
  m_hasExpectationForAddFailure = true;
  m_hasParametersExpectationForAddFailure = true;
  m_expectedAddFailureCallCount = 1;
  m_expectedFailedTest = failedTest;
  m_expectedException = thrownException;
  m_expectedIsError = isError;
}


void 
MockTestListener::setExpectNoFailure()
{
  m_hasExpectationForAddFailure = true;
  m_expectedAddFailureCallCount = 0;
}


void 
MockTestListener::setExpectFailure()
{
  m_hasExpectationForSomeFailure = true;
}


void 
MockTestListener::setExpectedAddFailureCall( int callCount )
{
  m_hasExpectationForAddFailure = true;
  m_expectedAddFailureCallCount = callCount;
}


void 
MockTestListener::setExpectStartTest( CppUnit::Test *test )
{
  m_hasExpectationForStartTest = true;
  m_hasParametersExpectationForStartTest = true;
  m_expectedStartTestCallCount = 1;
  m_expectedStartTest = test;
}


void 
MockTestListener::setExpectedStartTestCall( int callCount )
{
  m_hasExpectationForStartTest = true;
  m_expectedStartTestCallCount = callCount;
}


void 
MockTestListener::setExpectEndTest( CppUnit::Test *test )
{
  m_hasExpectationForEndTest = true;
  m_hasParametersExpectationForEndTest = true;
  m_expectedEndTestCallCount = 1;
  m_expectedEndTest = test;
}


void 
MockTestListener::setExpectedEndTestCall( int callCount )
{
  m_hasExpectationForEndTest = true;
  m_expectedEndTestCallCount = callCount;
}


void 
MockTestListener::addFailure( const CppUnit::TestFailure &failure )
{
  if ( m_hasExpectationForAddFailure  ||  m_hasExpectationForSomeFailure )
    ++m_addFailureCall;

  if ( m_hasExpectationForAddFailure )
  {
    CPPUNIT_ASSERT_MESSAGE( m_name + ": unexpected call",
                            m_addFailureCall <= m_expectedAddFailureCallCount );
  }

  if ( m_hasParametersExpectationForAddFailure )
  {
    CPPUNIT_ASSERT_MESSAGE( m_name + ": bad test",
                            m_expectedFailedTest == failure.failedTest() );
    CPPUNIT_ASSERT_MESSAGE( m_name + ": bad thrownException",
                            m_expectedException == failure.thrownException() );
    CPPUNIT_ASSERT_MESSAGE( m_name + ": bad isError",
                            m_expectedIsError == failure.isError() );
  }
}


void 
MockTestListener::startTest( CppUnit::Test *test )
{
  if ( m_hasExpectationForStartTest )
  {
    ++m_startTestCall;
    CPPUNIT_ASSERT_MESSAGE( m_name + ": unexpected call",
                            m_startTestCall <= m_expectedStartTestCallCount );
  
  }

  if ( m_hasParametersExpectationForStartTest )
  {
    CPPUNIT_ASSERT_MESSAGE( m_name + ": bad test",
                            m_expectedStartTest == test );
  }
}


void 
MockTestListener::endTest( CppUnit::Test *test )
{
  if ( m_hasExpectationForEndTest )
  {
    ++m_endTestCall;
    CPPUNIT_ASSERT_MESSAGE( m_name + ": unexpected call",
                            m_endTestCall <= m_expectedEndTestCallCount );
  }

  if ( m_hasParametersExpectationForEndTest )
  {
    CPPUNIT_ASSERT_MESSAGE( m_name + ": bad test",
                            m_expectedEndTest == test );
  }
}


void 
MockTestListener::verify()
{
  if ( m_hasExpectationForStartTest )
  {
    CPPUNIT_ASSERT_EQUAL_MESSAGE( m_name + ": missing startTest calls",
                                  m_expectedStartTestCallCount, 
                                  m_startTestCall );
  }

  if ( m_hasExpectationForEndTest )
  {
    CPPUNIT_ASSERT_EQUAL_MESSAGE( m_name + ": missing endTest calls",
                                  m_expectedEndTestCallCount, 
                                  m_endTestCall );
  }

  if ( m_hasExpectationForAddFailure )
  {
    CPPUNIT_ASSERT_EQUAL_MESSAGE( m_name + ": missing addFailure calls",
                                  m_expectedAddFailureCallCount, 
                                  m_addFailureCall );
  }

  if ( m_hasExpectationForSomeFailure )
  {
    CPPUNIT_ASSERT_MESSAGE( m_name + ": there was no call to "
                                "MockTestListener::addFailure()",
                            m_addFailureCall > 0 );
  }
}
