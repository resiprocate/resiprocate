#ifndef CPPUNITEST_XMLTESTRESULTOUTPUTTERTEST_H
#define CPPUNITEST_XMLTESTRESULTOUTPUTTERTEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResultCollector.h>
#include <deque>


/*! \class XmlOutputterTest
 * \brief Unit tests for XmlOutputter.
 */
class XmlOutputterTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( XmlOutputterTest );
  CPPUNIT_TEST( testEmptyNodeToString );
  CPPUNIT_TEST( testNodeWithAttributesToString );
  CPPUNIT_TEST( testEscapedAttributeValueToString );
  CPPUNIT_TEST( testNodeWithChildrenToString );
  CPPUNIT_TEST( testNodeWithContentToString );
  CPPUNIT_TEST( testNodeWithNumericContentToString );
  CPPUNIT_TEST( testNodeWithContentAndChildToString );
  CPPUNIT_TEST( testWriteXmlResultWithNoTest );
  CPPUNIT_TEST( testWriteXmlResultWithOneFailure );
  CPPUNIT_TEST( testWriteXmlResultWithOneError );
  CPPUNIT_TEST( testWriteXmlResultWithOneSucess );
  CPPUNIT_TEST( testWriteXmlResultWithThreeFailureTwoErrorsAndTwoSucess );
  CPPUNIT_TEST_SUITE_END();

public:
  /*! Constructs a XmlOutputterTest object.
   */
  XmlOutputterTest();

  /// Destructor.
  virtual ~XmlOutputterTest();

  void setUp();
  void tearDown();

  void testEmptyNodeToString();
  void testNodeWithAttributesToString();
  void testEscapedAttributeValueToString();
  void testNodeWithChildrenToString();
  void testNodeWithContentToString();
  void testNodeWithNumericContentToString();
  void testNodeWithContentAndChildToString();

  void testWriteXmlResultWithNoTest();
  void testWriteXmlResultWithOneFailure();
  void testWriteXmlResultWithOneError();
  void testWriteXmlResultWithOneSucess();
  void testWriteXmlResultWithThreeFailureTwoErrorsAndTwoSucess();

private:
  /// Prevents the use of the copy constructor.
  XmlOutputterTest( const XmlOutputterTest &copy );

  /// Prevents the use of the copy operator.
  void operator =( const XmlOutputterTest &copy );

  std::string statistics( int tests, 
                          int total, 
                          int error, 
                          int failure );

  void addTest( std::string testName );
  void addTestFailure( std::string testName,
                       std::string message,
                       CppUnit::SourceLine sourceLine = CppUnit::SourceLine() );
  void addTestError( std::string testName,
                     std::string message,
                     CppUnit::SourceLine sourceLine = CppUnit::SourceLine() );
  void addGenericTestFailure( std::string testName,
                              std::string message,
                              CppUnit::SourceLine sourceLine,
                              bool isError );

  CppUnit::Test *makeDummyTest( std::string testName );

private:
  CppUnit::TestResultCollector *m_result;
  std::deque<CppUnit::Test *> m_dummyTests;
};



#endif  // CPPUNITEST_XMLTESTRESULTOUTPUTTERTEST_H
