#include <cppunit/TestFailure.h>
#include <cppunit/TextTestProgressListener.h>
#include <iostream>
#include <cppunit/Test.h>


namespace CppUnit
{


TextTestProgressListener::TextTestProgressListener()
{
}


TextTestProgressListener::~TextTestProgressListener()
{
}


void 
TextTestProgressListener::startTest( Test *test )
{
  std::cerr << ". " << test->getName() << std::endl;
  std::cerr.flush();
}


void 
TextTestProgressListener::addFailure( const TestFailure &failure )
{
	std::cerr << ( failure.isError() ? "E " : "F " ) << failure.failedTestName() << std::endl;
  std::cerr.flush();
}


void 
TextTestProgressListener::done()
{
  std::cerr  <<  std::endl;
  std::cerr.flush();
}

} //  namespace CppUnit

