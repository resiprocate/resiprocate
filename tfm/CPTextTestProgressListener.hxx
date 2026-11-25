#ifndef CPPUNIT_CPTEXTTESTPROGRESSLISTENER_H
#define CPPUNIT_CPTEXTTESTPROGRESSLISTENER_H

#include <cppunit/TestListener.h>

#include <chrono>
#include <string>



/*! 
 * \brief TestListener that show the status of each TestCase test result.
 * \ingroup TrackingTestExecution
 */
class CPPUNIT_API CPTextTestProgressListener : public CppUnit::TestListener
{
public:
  /*! Constructs a CPTextTestProgressListener object.
   */
  CPTextTestProgressListener();

  /// Destructor.
  virtual ~CPTextTestProgressListener();

  void startTestRun( CppUnit::Test *test, CppUnit::TestResult *eventManager );
  void startTest( CppUnit::Test *test );
  void endTest( CppUnit::Test *test );
  void addFailure( const CppUnit::TestFailure &failure );

  void endTestRun( CppUnit::Test *test, 
                   CppUnit::TestResult *eventManager );

   void stopAllTestsUponFirstFailure(CppUnit::TestResult * pController);
private:
  /// Prevents the use of the copy constructor.
  CPTextTestProgressListener( const CPTextTestProgressListener &copy );

  /// Prevents the use of the copy operator.
  void operator =( const CPTextTestProgressListener &copy );

private:
   std::string progress() const;  // e.g. "12/168"

   CppUnit::TestResult * mTestController;
   int mTestCount;    // tests in this run
   int mTestNumber;   // the test now running, from 1
   std::chrono::steady_clock::time_point mTestStart;
};


#endif  // CPPUNIT_TEXTTESTPROGRESSLISTENER_H
