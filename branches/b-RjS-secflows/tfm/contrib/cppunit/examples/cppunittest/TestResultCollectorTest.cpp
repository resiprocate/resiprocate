#include "CoreSuite.h"
#include "TestResultCollectorTest.h"



CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestResultCollectorTest,
                                       CppUnitTest::coreSuiteName() );


TestResultCollectorTest::TestResultCollectorTest()
{
}


TestResultCollectorTest::~TestResultCollectorTest()
{
}


void 
TestResultCollectorTest::setUp()
{
  m_lockCount = 0;
  m_unlockCount = 0;
  m_result = new CppUnit::TestResultCollector();
  m_synchronizedResult = new SynchronizedTestResult( this );  
  m_test = new CppUnit::TestCase();
  m_test2 = new CppUnit::TestCase();
}


void 
TestResultCollectorTest::tearDown()
{
  delete m_test2;
  delete m_test;
  delete m_synchronizedResult;
  delete m_result;
}


void 
TestResultCollectorTest::testConstructor()
{
  checkResult( 0, 0, 0 );
}


void 
TestResultCollectorTest::testAddTwoErrors()
{
  std::string errorMessage1( "First Error" );
  std::string errorMessage2( "Second Error" );
  {
    CppUnit::TestFailure failure1( m_test, 
                                   new CppUnit::Exception( errorMessage1 ),
                                   true );
    m_result->addFailure( failure1 );

    CppUnit::TestFailure failure2( m_test2, 
                                   new CppUnit::Exception( errorMessage2 ),
                                   true );
    m_result->addFailure( failure2 );
  } // ensure that the test result duplicate the failures.

  checkResult( 0, 2, 0 );
  checkFailure( m_result->failures()[0],
                errorMessage1,
                m_test,
                true );
  checkFailure( m_result->failures()[1],
                errorMessage2,
                m_test2,
                true );
}


void 
TestResultCollectorTest::testAddTwoFailures()
{
  std::string errorMessage1( "First Failure" );
  std::string errorMessage2( "Second Failure" );
  {
    CppUnit::TestFailure failure1( m_test, 
                                   new CppUnit::Exception( errorMessage1 ),
                                   false );
    m_result->addFailure( failure1 );

    CppUnit::TestFailure failure2( m_test2, 
                                   new CppUnit::Exception( errorMessage2 ),
                                   false );
    m_result->addFailure( failure2 );
  } // ensure that the test result duplicate the failures.
  checkResult( 2, 0, 0 );
  checkFailure( m_result->failures()[0],
                errorMessage1,
                m_test,
                false );
  checkFailure( m_result->failures()[1],
                errorMessage2,
                m_test2,
                false );
}


void 
TestResultCollectorTest::testStartTest()
{
  m_result->startTest( m_test );
  m_result->startTest( m_test );
  checkResult( 0, 0, 2 );
}


void 
TestResultCollectorTest::testWasSuccessfulWithNoTest()
{
  checkWasSuccessful( true );
}


void 
TestResultCollectorTest::testWasSuccessfulWithErrors()
{
  addError( "Error1" );
  addError( "Error2" );
  checkWasSuccessful( false );
}


void 
TestResultCollectorTest::testWasSuccessfulWithFailures()
{
  addFailure( "Failure1" );
  addFailure( "Failure2" );
  checkWasSuccessful( false );
}


void 
TestResultCollectorTest::testWasSuccessfulWithErrorsAndFailures()
{
  addError( "Error1" );
  addFailure( "Failure2" );
  checkWasSuccessful( false );
}


void 
TestResultCollectorTest::testWasSuccessfulWithSucessfulTest()
{
  m_result->startTest( m_test );
  m_result->endTest( m_test );
  m_result->startTest( m_test2 );
  m_result->endTest( m_test2 );
  checkWasSuccessful( true );
}


void 
TestResultCollectorTest::testSynchronizationAddFailure()
{
  addFailure( "Failure1", m_test, false, m_synchronizedResult );
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationStartTest()
{
  m_synchronizedResult->startTest( m_test );
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationRunTests()
{
  m_synchronizedResult->runTests();
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationTestErrors()
{
  m_synchronizedResult->testErrors();
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationTestFailures()
{
  m_synchronizedResult->testFailures();
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationFailures()
{
  m_synchronizedResult->failures();
  checkSynchronization();
}


void 
TestResultCollectorTest::testSynchronizationWasSuccessful()
{
  m_synchronizedResult->wasSuccessful();
  checkSynchronization();
}


void 
TestResultCollectorTest::checkResult( int failures,
                             int errors,
                             int testsRun )
{
  CPPUNIT_ASSERT_EQUAL( testsRun, m_result->runTests() );
  CPPUNIT_ASSERT_EQUAL( errors, m_result->testErrors() );
  CPPUNIT_ASSERT_EQUAL( failures, m_result->testFailures() );
  CPPUNIT_ASSERT_EQUAL( errors + failures, 
                        m_result->testFailuresTotal() );
}


void
TestResultCollectorTest::checkFailure( CppUnit::TestFailure *failure,
                                       std::string expectedMessage,
                                       CppUnit::Test *expectedTest,
                                       bool expectedIsError )
{
  std::string actualMessage( failure->thrownException()->what() );
  CPPUNIT_ASSERT_EQUAL( expectedMessage, actualMessage );
  CPPUNIT_ASSERT_EQUAL( expectedTest, failure->failedTest() );
  CPPUNIT_ASSERT_EQUAL( expectedIsError, failure->isError() );
}


void 
TestResultCollectorTest::checkWasSuccessful( bool shouldBeSuccessful )
{
  CPPUNIT_ASSERT_EQUAL( shouldBeSuccessful, m_result->wasSuccessful() );
}


void 
TestResultCollectorTest::locked()
{
  CPPUNIT_ASSERT_EQUAL( m_lockCount, m_unlockCount );
  ++m_lockCount;
}


void 
TestResultCollectorTest::unlocked()
{
  ++m_unlockCount;
  CPPUNIT_ASSERT_EQUAL( m_lockCount, m_unlockCount );
}


void 
TestResultCollectorTest::checkSynchronization()
{
  CPPUNIT_ASSERT_EQUAL( m_lockCount, m_unlockCount );
  CPPUNIT_ASSERT( m_lockCount > 0 );
}


void 
TestResultCollectorTest::addFailure( std::string message )
{
  addFailure( message, m_test, false, m_result );
}


void 
TestResultCollectorTest::addError( std::string message )
{
  addFailure( message, m_test, true, m_result );
}


void 
TestResultCollectorTest::addFailure( std::string message, 
                                     CppUnit::Test *failedTest, 
                                     bool isError,
                                     CppUnit::TestResultCollector *result )
{
  CppUnit::TestFailure failure( failedTest, 
                                new CppUnit::Exception( message ), 
                                isError );
  result->addFailure( failure );
}
