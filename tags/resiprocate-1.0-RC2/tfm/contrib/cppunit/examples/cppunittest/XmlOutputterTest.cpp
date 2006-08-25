#include <cppunit/XmlOutputter.h>
#include <cppunit/TestFailure.h>
#include <cppunit/XmlOutputter.h>
#include "OutputSuite.h"
#include "XmlOutputterTest.h"
#include "XmlUniformiser.h"


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( XmlOutputterTest, 
                                       CppUnitTest::outputSuiteName() );


XmlOutputterTest::XmlOutputterTest()
{
}


XmlOutputterTest::~XmlOutputterTest()
{
}


void 
XmlOutputterTest::setUp()
{
  m_dummyTests.clear();
  m_result = new CppUnit::TestResultCollector();
}


void 
XmlOutputterTest::tearDown()
{
  delete m_result;
  for ( int index =0; index < m_dummyTests.size(); ++index )
    delete m_dummyTests[index];
  m_dummyTests.clear();
}


void 
XmlOutputterTest::testEmptyNodeToString()
{
  CppUnit::XmlOutputter::Node node( "element" );
  std::string expectedXml = "<element></element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testNodeWithAttributesToString()
{
  CppUnit::XmlOutputter::Node node( "element" );
  node.addAttribute( "id", 17 );
  node.addAttribute( "date-format", "iso-8901" );
  std::string expectedXml = "<element id=\"17\" "
                            "date-format=\"iso-8901\">"
                            "</element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testEscapedAttributeValueToString()
{
  CppUnit::XmlOutputter::Node node( "element" );
  node.addAttribute( "escaped", "&<>\"'" );
  std::string expectedXml = "<element escaped=\""
                            "&amp;&lt;&gt;&quot;&apos;"
                            "\"></element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testNodeWithChildrenToString()
{
  CppUnit::XmlOutputter::Node node( "element" );
  node.addNode( new CppUnit::XmlOutputter::Node( "child1" ) );
  node.addNode( new CppUnit::XmlOutputter::Node( "child2" ) );
  std::string expectedXml = "<element><child1></child1>"
                            "<child2></child2></element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testNodeWithContentToString()
{
  CppUnit::XmlOutputter::Node node( "element", "content\nline2" );
  std::string expectedXml = "<element>content\nline2</element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testNodeWithNumericContentToString()
{
  CppUnit::XmlOutputter::Node node( "element", 123456789 );
  std::string expectedXml = "<element>123456789</element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testNodeWithContentAndChildToString()
{
  CppUnit::XmlOutputter::Node node( "element", "content" );
  node.addNode( new CppUnit::XmlOutputter::Node( "child1" ) );
  std::string expectedXml = "<element><child1></child1>content</element>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, node.toString() );
}


void 
XmlOutputterTest::testWriteXmlResultWithNoTest()
{
  CppUnit::OStringStream stream;
  CppUnit::XmlOutputter outputter( m_result, stream );
  outputter.write();

  std::string actualXml = stream.str();
  std::string expectedXml = 
    "<TestRun>"
      "<FailedTests></FailedTests>"
      "<SucessfulTests></SucessfulTests>"
      "<Statistics>"
        "<Tests>0</Tests>"
        "<FailuresTotal>0</FailuresTotal>"
        "<Errors>0</Errors>"
        "<Failures>0</Failures>"
      "</Statistics>"
    "</TestRun>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, actualXml );
}


void 
XmlOutputterTest::testWriteXmlResultWithOneFailure()
{
  addTestFailure( "test1", "message failure1", CppUnit::SourceLine( "test.cpp", 3 ) );

  CppUnit::OStringStream stream;
  CppUnit::XmlOutputter outputter( m_result, stream );
  outputter.write();

  std::string actualXml = stream.str();
  std::string expectedXml = 
    "<TestRun>"
      "<FailedTests>"
        "<FailedTest id=\"1\">"
          "<Name>test1</Name>"
          "<FailureType>Assertion</FailureType>"
          "<Location>"
            "<File>test.cpp</File>"
            "<Line>3</Line>"
          "</Location>"
          "message failure1"
        "</FailedTest>"
      "</FailedTests>"
      "<SucessfulTests></SucessfulTests>"
      "<Statistics>"
        "<Tests>1</Tests>"
        "<FailuresTotal>1</FailuresTotal>"
        "<Errors>0</Errors>"
        "<Failures>1</Failures>"
      "</Statistics>"
    "</TestRun>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, actualXml );
}


void 
XmlOutputterTest::testWriteXmlResultWithOneError()
{
  addTestError( "test1", "message error1" );

  CppUnit::OStringStream stream;
  CppUnit::XmlOutputter outputter( m_result, stream );
  outputter.write();

  std::string actualXml = stream.str();
  std::string expectedXml = 
    "<TestRun>"
      "<FailedTests>"
        "<FailedTest id=\"1\">"
          "<Name>test1</Name>"
          "<FailureType>Error</FailureType>"
          "message error1"
        "</FailedTest>"
      "</FailedTests>"
      "<SucessfulTests></SucessfulTests>"
      "<Statistics>"
        "<Tests>1</Tests>"
        "<FailuresTotal>1</FailuresTotal>"
        "<Errors>1</Errors>"
        "<Failures>0</Failures>"
      "</Statistics>"
    "</TestRun>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, actualXml );
}


void 
XmlOutputterTest::testWriteXmlResultWithOneSucess()
{
  addTest( "test1" );

  CppUnit::OStringStream stream;
  CppUnit::XmlOutputter outputter( m_result, stream );
  outputter.write();

  std::string actualXml = stream.str();
  std::string expectedXml = 
    "<TestRun>"
      "<FailedTests></FailedTests>"
      "<SucessfulTests>"
        "<Test id=\"1\">"
          "<Name>test1</Name>"
        "</Test>"
      "</SucessfulTests>"
      "<Statistics>"
        "<Tests>1</Tests>"
        "<FailuresTotal>0</FailuresTotal>"
        "<Errors>0</Errors>"
        "<Failures>0</Failures>"
      "</Statistics>"
    "</TestRun>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, actualXml );
}


void 
XmlOutputterTest::testWriteXmlResultWithThreeFailureTwoErrorsAndTwoSucess()
{
  addTestFailure( "test1", "failure1" );
  addTestError( "test2", "error1" );
  addTestFailure( "test3", "failure2" );
  addTestFailure( "test4", "failure3" );
  addTest( "test5" );
  addTestError( "test6", "error2" );
  addTest( "test7" );

  CppUnit::OStringStream stream;
  CppUnit::XmlOutputter outputter( m_result, stream );
  outputter.write();

  std::string actualXml = stream.str();
  std::string expectedXml = 
    "<TestRun>"
       "<FailedTests>"
        "<FailedTest id=\"1\">"
          "<Name>test1</Name>"
          "<FailureType>Assertion</FailureType>"
          "failure1"
        "</FailedTest>"
        "<FailedTest id=\"2\">"
          "<Name>test2</Name>"
          "<FailureType>Error</FailureType>"
          "error1"
        "</FailedTest>"
        "<FailedTest id=\"3\">"
          "<Name>test3</Name>"
          "<FailureType>Assertion</FailureType>"
          "failure2"
        "</FailedTest>"
        "<FailedTest id=\"4\">"
          "<Name>test4</Name>"
          "<FailureType>Assertion</FailureType>"
          "failure3"
        "</FailedTest>"
        "<FailedTest id=\"6\">"
          "<Name>test6</Name>"
          "<FailureType>Error</FailureType>"
          "error2"
        "</FailedTest>"
      "</FailedTests>"
     "<SucessfulTests>"
        "<Test id=\"5\">"
          "<Name>test5</Name>"
        "</Test>"
        "<Test id=\"7\">"
          "<Name>test7</Name>"
        "</Test>"
      "</SucessfulTests>"
      "<Statistics>"
        "<Tests>7</Tests>"
        "<FailuresTotal>5</FailuresTotal>"
        "<Errors>2</Errors>"
        "<Failures>3</Failures>"
      "</Statistics>"
    "</TestRun>";
  CPPUNITTEST_ASSERT_XML_EQUAL( expectedXml, actualXml );
}


void 
XmlOutputterTest::addTest( std::string testName )
{
  CppUnit::Test *test = makeDummyTest( testName );
  m_result->startTest( test );
  m_result->endTest( test );
}


void 
XmlOutputterTest::addTestFailure( std::string testName,
                                  std::string message,
                                  CppUnit::SourceLine sourceLine )
{
  addGenericTestFailure( testName, message, sourceLine, false );
}


void 
XmlOutputterTest::addTestError( std::string testName,
                                std::string message,
                                CppUnit::SourceLine sourceLine )
{
  addGenericTestFailure( testName, message, sourceLine, true );
}


void 
XmlOutputterTest::addGenericTestFailure(  std::string testName,
                                          std::string message,
                                          CppUnit::SourceLine sourceLine,
                                          bool isError )
{
  CppUnit::Test *test = makeDummyTest( testName );
  m_result->startTest( test );
  CppUnit::TestFailure failure( test, 
                                new CppUnit::Exception( message, sourceLine ),
                                isError );
  m_result->addFailure( failure );
  m_result->endTest( test );
}


CppUnit::Test *
XmlOutputterTest::makeDummyTest( std::string testName )
{
  CppUnit::Test *test = new CppUnit::TestCase( testName );
  m_dummyTests.push_back( test );
  return test;
}
