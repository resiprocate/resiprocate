#include <cppunit/TestFailure.h>
#include <cppunit/Test.h>
#include <cppunit/SourceLine.h>
#include <cppunit/portability/Stream.h>
#include <cppunit/Exception.h>

#include <cppunit/TestResult.h>

#include "CPTextTestProgressListener.hxx"



CPTextTestProgressListener::CPTextTestProgressListener()
{
   mTestController = 0;
}


CPTextTestProgressListener::~CPTextTestProgressListener()
{
}

void
CPTextTestProgressListener::stopAllTestsUponFirstFailure(CppUnit::TestResult * pController)
{
   mTestController = pController;
}

void 
CPTextTestProgressListener::startTest( CppUnit::Test *test )
{
	CppUnit::stdCOut() << "Starting test:  " << test->getName() << std::endl;
}


void 
CPTextTestProgressListener::addFailure( const CppUnit::TestFailure &failure )
{
	CppUnit::SourceLine srcLn = failure.sourceLine(); 
	
	CppUnit::stdCOut() << std::endl << ( failure.isError() ? "Test Error:  " : "Test Failed:  " ) << failure.failedTestName() << std::endl;

	CppUnit::stdCOut() << "Line: " << srcLn.lineNumber() << "  " << srcLn.fileName() << std::endl;

	CppUnit::stdCOut()  <<  failure.thrownException()->message().shortDescription()  << std::endl;
        
        CppUnit::stdCOut()  <<  failure.thrownException()->message().details() << std::endl;

   if (mTestController)
      mTestController->stop();
}


void 
CPTextTestProgressListener::endTestRun( CppUnit::Test *test, 
                                      CppUnit::TestResult *eventManager )
{
  CppUnit::stdCOut() << std::endl << "Done:  " << test->getName() << std::endl;
  CppUnit::stdCOut().flush();
}



