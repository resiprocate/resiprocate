#ifndef TESTSETUPTEST_H
#define TESTSETUPTEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestSetUp.h>


class TestSetUpTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestSetUpTest );
  CPPUNIT_TEST( testRun );
  CPPUNIT_TEST_SUITE_END();

public:
  TestSetUpTest();
  virtual ~TestSetUpTest();

  void setUp();
  void tearDown();

  void testRun();

private:
  class MockSetUp : public CppUnit::TestSetUp
  {
  public:
    MockSetUp( CppUnit::Test *test )
        : CppUnit::TestSetUp( test )
        , m_setUpCalled( false )
        , m_tearDownCalled( false )
    {
    }

    void setUp() 
    {
      m_setUpCalled = true;
    }

    void tearDown()
    {
      m_tearDownCalled = true;
    }

    void verify()
    {
      CPPUNIT_ASSERT( m_setUpCalled );
      CPPUNIT_ASSERT( m_tearDownCalled );
    }

  private:
    bool m_setUpCalled;
    bool m_tearDownCalled;
  };

  TestSetUpTest( const TestSetUpTest &copy );
  void operator =( const TestSetUpTest &copy );

private:
};



#endif  // TESTSETUPTEST_H
