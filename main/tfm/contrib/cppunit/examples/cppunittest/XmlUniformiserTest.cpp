#include <cppunit/NotEqualException.h>
#include <stdlib.h>
#include "UnitTestToolSuite.h"
#include "XmlUniformiserTest.h"
#include "XmlUniformiser.h"

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( XmlUniformiserTest, 
                                       CppUnitTest::unitTestToolSuiteName() );


XmlUniformiserTest::XmlUniformiserTest()
{
}


XmlUniformiserTest::~XmlUniformiserTest()
{
}


void 
XmlUniformiserTest::setUp()
{
}


void 
XmlUniformiserTest::tearDown()
{
}


void 
XmlUniformiserTest::testEmpty()
{
  check( "", "" );
}


void 
XmlUniformiserTest::testSkipProcessed()
{
  check( "<?xml version=\"1.0\" encoding='ISO-8859-1' ?>",
         "" );
}


void 
XmlUniformiserTest::testOpenElementWithoutAttributeButSomeSpaces()
{
  check( "   <Test   >   ", 
         "<Test>" );
}


void 
XmlUniformiserTest::testOpenCloseElement()
{
  check( "  <TestName  >   </TestName  >  ",
         "<TestName></TestName>" );
}


void 
XmlUniformiserTest::testElementWithEmptyAttribute()
{
  check( "<TestName id=\"\">",
         "<TestName id=\"\">" );
}


void 
XmlUniformiserTest::testElementWithEmptyAttributeButSomeSpaces()
{
  check( "<TestName  id  = \"\" >",
         "<TestName id=\"\">" );
}


void 
XmlUniformiserTest::testElementWithOneAttribute()
{
  check( "<FailedTest id=\"123\">",
         "<FailedTest id=\"123\">" );
}


void 
XmlUniformiserTest::testElementWithThreeAttributes()
{
  check( "<FailedTest  id = \"7\" date-format= \"iso-8901\"  "
                                           "style =\"debug\">",
         "<FailedTest id=\"7\" date-format=\"iso-8901\" style=\"debug\">" );
}


void 
XmlUniformiserTest::testElementWithContent()
{
  check( "<Element>\nContent\n</Element>\n",
         "<Element>Content\n</Element>" );
}


void 
XmlUniformiserTest::testElementsHierarchyWithContents()
{
  check( "<TestRuns date-format=\"iso-8901\">\n"
            "<Date>2001-10-04</Date>\n"
            "<FailedTests>\n<FailedTest>\n"
            "<TestName>TokenParserTest</TestName>\n"
            "</FailedTest>\n</Failedtests>\n</TestRuns>\n",
         "<TestRuns date-format=\"iso-8901\">"
            "<Date>2001-10-04</Date>"
            "<FailedTests><FailedTest>"
            "<TestName>TokenParserTest</TestName>"
            "</FailedTest></Failedtests></TestRuns>" );
}


void 
XmlUniformiserTest::testSkipComment()
{
  check( "<!-- skip comment-->",
         "" );
}


void 
XmlUniformiserTest::testAssertXmlEqualFail()
{
  try
  {
    CPPUNITTEST_ASSERT_XML_EQUAL( "<Test>", "<Tes>" );
    CPPUNIT_FAIL( "CppUnit::NotEqualException expected" );
  }
  catch ( CppUnit::NotEqualException &e )
  {
    CPPUNIT_ASSERT_EQUAL( e.expectedValue(), std::string("<Test>" ) );
    CPPUNIT_ASSERT_EQUAL( e.actualValue(), std::string("<Tes>" ) );
    CPPUNIT_ASSERT( !e.additionalMessage().empty() );
    CPPUNIT_ASSERT( e.sourceLine().isValid() );
  }
}

void 
XmlUniformiserTest::testAssertXmlEqualPass()
{
  CPPUNITTEST_ASSERT_XML_EQUAL( "<Test>", "<Test>" );
}


void 
XmlUniformiserTest::check( const std::string &xml, 
                           const std::string &expectedStrippedXml )
{
  std::string actual = CppUnitTest::XmlUniformiser( xml ).stripped();
  CPPUNIT_ASSERT_EQUAL( expectedStrippedXml, actual );
}
