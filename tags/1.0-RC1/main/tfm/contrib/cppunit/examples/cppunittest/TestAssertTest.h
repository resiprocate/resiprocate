#ifndef TESTASSERTTEST_H
#define TESTASSERTTEST_H

#include <cppunit/extensions/HelperMacros.h>


class TestAssertTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestAssertTest );
  CPPUNIT_TEST( testAssertTrue );
  CPPUNIT_TEST_FAIL( testAssertFalse );
  CPPUNIT_TEST( testAssertEqual );
  CPPUNIT_TEST( testAssertMessageTrue );
  CPPUNIT_TEST( testAssertMessageFalse );
  CPPUNIT_TEST( testAssertDoubleEquals );
  CPPUNIT_TEST_FAIL( testAssertDoubleNotEquals1 );
  CPPUNIT_TEST_FAIL( testAssertDoubleNotEquals2 );
  CPPUNIT_TEST( testAssertLongEquals );
  CPPUNIT_TEST_FAIL( testAssertLongNotEquals );
  CPPUNIT_TEST( testFail );
  CPPUNIT_TEST_SUITE_END();

public:
  TestAssertTest();

  virtual ~TestAssertTest();

  virtual void setUp();
  virtual void tearDown();

  void testAssertTrue();
  void testAssertFalse();
  
  void testAssertEqual();

  void testAssertMessageTrue();
  void testAssertMessageFalse();

  void testAssertDoubleEquals();
  void testAssertDoubleNotEquals1();
  void testAssertDoubleNotEquals2();

  void testAssertLongEquals();
  void testAssertLongNotEquals();

  void testFail();

private:
  TestAssertTest( const TestAssertTest &copy );
  void operator =( const TestAssertTest &copy );

  void checkDoubleNotEquals( double expected, 
                             double actual, 
                             double delta );

  void checkMessageContains( CppUnit::Exception *e,
                             std::string expectedMessage );

private:
};

#endif  // TESTASSERTTEST_H
