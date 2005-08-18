#ifndef TESTRESULTTEST_H
#define TESTRESULTTEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestResult.h>
#include "MockTestListener.h"


class TestResultTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestResultTest );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testStop );
  CPPUNIT_TEST( testAddError );
  CPPUNIT_TEST( testAddFailure );
  CPPUNIT_TEST( testStartTest );
  CPPUNIT_TEST( testEndTest );
  CPPUNIT_TEST( testTwoListener );
  CPPUNIT_TEST_SUITE_END();

public:
  TestResultTest();
  virtual ~TestResultTest();

  virtual void setUp();
  virtual void tearDown();

  void testConstructor();
  void testStop();

  void testAddError();
  void testAddFailure();
  void testStartTest();
  void testEndTest();

  void testNoListener();
  void testTwoListener();

  void testRemoveLastListener();
  void testRemoveFrontListener();

private:
  TestResultTest( const TestResultTest &copy );
  void operator =( const TestResultTest &copy );

private:
  CppUnit::TestResult *m_result;
  MockTestListener *m_listener1;
  MockTestListener *m_listener2;
  CppUnit::Test *m_dummyTest;
};



#endif  // TESTRESULTTEST_H
