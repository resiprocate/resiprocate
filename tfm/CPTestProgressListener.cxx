#include <cppunit/TestFailure.h>
#include <cppunit/Test.h>
#include <cppunit/SourceLine.h>
#include <cppunit/portability/Stream.h>
#include <cppunit/Exception.h>

#include <cppunit/TestResult.h>

#include "CPTextTestProgressListener.hxx"



CPTextTestProgressListener::CPTextTestProgressListener()
   : mTestController(0),
     mTestCount(0),
     mTestNumber(0)
{
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
CPTextTestProgressListener::startTestRun( CppUnit::Test *test, CppUnit::TestResult * /*eventManager*/ )
{
   mTestCount = test->countTestCases();
   mTestNumber = 0;
}

// Each line is flushed as it is written, so that progress can be followed while the tests run (e.g. under ctest --verbose)
void
CPTextTestProgressListener::startTest( CppUnit::Test *test )
{
   ++mTestNumber;
   mTestStart = std::chrono::steady_clock::now();
   CppUnit::stdCOut() << "Starting test " << progress() << ":  " << test->getName() << std::endl;
}

void
CPTextTestProgressListener::endTest( CppUnit::Test *test )
{
   const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mTestStart).count();
   CppUnit::stdCOut() << "Finished test " << progress() << ":  " << test->getName()
                      << " (" << elapsedMs / 1000 << "." << (elapsedMs % 1000) / 100 << " s)" << std::endl;
}

std::string
CPTextTestProgressListener::progress() const
{
   // The number of tests is only known when the listener is told about the start of the run
   return std::to_string(mTestNumber) + (mTestCount > 0 ? "/" + std::to_string(mTestCount) : std::string());
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



