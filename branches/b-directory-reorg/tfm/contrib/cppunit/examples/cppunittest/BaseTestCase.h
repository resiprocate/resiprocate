#ifndef BASETESTCASE_H
#define BASETESTCASE_H

#include <cppunit/extensions/HelperMacros.h>


class BaseTestCase : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE( BaseTestCase );
  CPPUNIT_TEST( testUsingCheckIt );
  CPPUNIT_TEST_SUITE_END();

public:
  BaseTestCase();
  virtual ~BaseTestCase();

  virtual void setUp();
  virtual void tearDown();

  void testUsingCheckIt();

protected:
  virtual void checkIt();

private:
  BaseTestCase( const BaseTestCase &copy );
  void operator =( const BaseTestCase &copy );
};



#endif  // BASETESTCASE_H
