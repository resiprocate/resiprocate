#ifndef CPPUNIT_CPTEXTTESTPROGRESSLISTENER_H
#define CPPUNIT_CPTEXTTESTPROGRESSLISTENER_H

#include <cppunit/TestListener.h>



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

  void startTest( CppUnit::Test *test );
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
   CppUnit::TestResult * mTestController;
};


#endif  // CPPUNIT_TEXTTESTPROGRESSLISTENER_H
