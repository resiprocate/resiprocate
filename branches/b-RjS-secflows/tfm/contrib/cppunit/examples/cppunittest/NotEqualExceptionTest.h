#ifndef NOTEQUALEXCEPTIONTEST_H
#define NOTEQUALEXCEPTIONTEST_H

#include <cppunit/extensions/HelperMacros.h>


class NotEqualExceptionTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( NotEqualExceptionTest );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testClone );
  CPPUNIT_TEST( testIsInstanceOf );
  CPPUNIT_TEST( testAssignment );
  CPPUNIT_TEST_SUITE_END();

public:
  NotEqualExceptionTest();
  virtual ~NotEqualExceptionTest();

  virtual void setUp();
  virtual void tearDown();

  void testConstructor();
  void testClone();
  void testIsInstanceOf();
  void testAssignment();

private:
  NotEqualExceptionTest( const NotEqualExceptionTest &copy );
  void operator =( const NotEqualExceptionTest &copy );

private:
};



#endif  // NOTEQUALEXCEPTIONTEST_H
