#include "CoreSuite.h"
#include "ExceptionTest.h"
#include <cppunit/Exception.h>
#include <cppunit/NotEqualException.h>
#include <memory>


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( ExceptionTest,
                                       CppUnitTest::coreSuiteName() );


ExceptionTest::ExceptionTest()
{
}


ExceptionTest::~ExceptionTest()
{
}


void 
ExceptionTest::setUp()
{
}


void 
ExceptionTest::tearDown()
{
}


void 
ExceptionTest::testConstructor()
{
  const std::string message( "a message" );
  const CppUnit::SourceLine sourceLine( "dir/afile.cpp", 17 );
  
  CppUnit::Exception e( message, sourceLine );

  CPPUNIT_ASSERT_EQUAL( message, std::string( e.what() ) );
  CPPUNIT_ASSERT( sourceLine == e.sourceLine() );
}


void 
ExceptionTest::testDefaultConstructor()
{
  CppUnit::Exception e;

  CPPUNIT_ASSERT_EQUAL( std::string(""), std::string( e.what() ) );
  CPPUNIT_ASSERT( !e.sourceLine().isValid() );
}


void 
ExceptionTest::testCopyConstructor()
{
  CppUnit::SourceLine sourceLine( "fileName.cpp", 123 );
  CppUnit::Exception e( "message", sourceLine  );
  CppUnit::Exception other( e );
  checkIsSame( e, other );
}


void 
ExceptionTest::testAssignment()
{
  CppUnit::SourceLine sourceLine( "fileName.cpp", 123 );
  CppUnit::Exception e( "message", sourceLine  );
  CppUnit::Exception other;
  other = e;
  checkIsSame( e, other );
}


void 
ExceptionTest::testClone()
{
  CppUnit::SourceLine sourceLine( "fileName.cpp", 123 );
  CppUnit::Exception e( "message", sourceLine  );
  std::auto_ptr<CppUnit::Exception> other( e.clone() );
  checkIsSame( e, *other.get() );
}


void 
ExceptionTest::testIsInstanceOf()
{
  CppUnit::SourceLine sourceLine( "fileName.cpp", 123 );
  CppUnit::Exception e( "message", sourceLine  );
  CPPUNIT_ASSERT( e.isInstanceOf( CppUnit::Exception::type() ) );
  CPPUNIT_ASSERT( !e.isInstanceOf( CppUnit::NotEqualException::type() ) );
}


void 
ExceptionTest::checkIsSame( CppUnit::Exception &e, 
                            CppUnit::Exception &other )
{
  std::string eWhat( e.what() );
  std::string otherWhat( other.what() );
  CPPUNIT_ASSERT_EQUAL( eWhat, otherWhat );
  CPPUNIT_ASSERT( e.sourceLine() == other.sourceLine() );
}
