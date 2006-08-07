#ifndef EXCEPTIONTEST_H
#define EXCEPTIONTEST_H

#include <cppunit/extensions/HelperMacros.h>


class ExceptionTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ExceptionTest );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testDefaultConstructor );
  CPPUNIT_TEST( testCopyConstructor );
  CPPUNIT_TEST( testAssignment );
  CPPUNIT_TEST( testClone );
  CPPUNIT_TEST( testIsInstanceOf );
  CPPUNIT_TEST_SUITE_END();

public:
  ExceptionTest();
  virtual ~ExceptionTest();

  virtual void setUp();
  virtual void tearDown();

  void testConstructor();
  void testDefaultConstructor();
  void testCopyConstructor();
  void testAssignment();
  void testClone();
  void testIsInstanceOf();

private:
  ExceptionTest( const ExceptionTest &copy );
  void operator =( const ExceptionTest &copy );
  void checkIsSame( CppUnit::Exception &e, 
                    CppUnit::Exception &other );

private:
};



#endif  // EXCEPTIONTEST_H
