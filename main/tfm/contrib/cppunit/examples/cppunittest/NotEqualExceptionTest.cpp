#include "CoreSuite.h"
#include "NotEqualExceptionTest.h"
#include <cppunit/NotEqualException.h>


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( NotEqualExceptionTest,
                                       CppUnitTest::coreSuiteName() );


NotEqualExceptionTest::NotEqualExceptionTest()
{
}


NotEqualExceptionTest::~NotEqualExceptionTest()
{
}


void 
NotEqualExceptionTest::setUp()
{
}


void 
NotEqualExceptionTest::tearDown()
{
}



void 
NotEqualExceptionTest::testConstructor()
{
  std::string expectedValue( "expectedValue" );
  std::string actualValue( "actualValue" );
  std::string message( "message" );
  CppUnit::NotEqualException e( expectedValue, 
                                actualValue, 
                                CppUnit::SourceLine( "f.cpp", 123), 
                                message );

  CPPUNIT_ASSERT_EQUAL( expectedValue, e.expectedValue() );
  CPPUNIT_ASSERT_EQUAL( actualValue, e.actualValue() );
  CPPUNIT_ASSERT_EQUAL( message, e.additionalMessage() );
  CPPUNIT_ASSERT( CppUnit::SourceLine( "f.cpp", 123) == e.sourceLine() );

  std::string expectedMessage( "Expected: expectedValue, but was: actualValue.message" );
  std::string actualMessage( e.what() );
  CPPUNIT_ASSERT_EQUAL( expectedMessage, actualMessage );
}


void 
NotEqualExceptionTest::testClone()
{
  CppUnit::NotEqualException e( "expectedValue", 
                                "actualValue", 
                                CppUnit::SourceLine("file.cpp", 2),
                                "add-message" );
  std::auto_ptr<CppUnit::NotEqualException> 
      other( (CppUnit::NotEqualException *)e.clone() );
  CPPUNIT_ASSERT_EQUAL( std::string( e.what() ),
                        std::string( other->what() ) );
}


void 
NotEqualExceptionTest::testIsInstanceOf()
{
  CppUnit::NotEqualException e( "expectedValue", "actualValue" );
  CPPUNIT_ASSERT( e.isInstanceOf( CppUnit::NotEqualException::type() ) );
  CPPUNIT_ASSERT( e.isInstanceOf( CppUnit::Exception::type() ) );
}


void 
NotEqualExceptionTest::testAssignment()
{
  CppUnit::NotEqualException e( "expectedValue", 
                                "actualValue", 
                                CppUnit::SourceLine("file.cpp", 2),
                                "add-message" );
  CppUnit::NotEqualException other( "", "" );
  other = e;
  CPPUNIT_ASSERT_EQUAL( std::string( e.what() ),
                        std::string( other.what() ) );
}
