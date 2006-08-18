

#ifndef CPPUNIT_ACTIVETEST_H
#define CPPUNIT_ACTIVETEST_H

#include <afxmt.h>

#ifndef CPPUNIT_TESTDECORATOR_H
#include <cppunit/extensions/TestDecorator.h>
#endif


/* A Microsoft-specific active test
 *
 * An active test manages its own
 * thread of execution.  This one 
 * is very simple and only sufficient
 * for the limited use we put it through
 * in the TestRunner.  It spawns a thread
 * on run (TestResult *) and signals
 * completion of the test.
 *
 * We assume that only one thread 
 * will be active at once for each
 * instance.
 *
 */

class ActiveTest : public CppUnit::TestDecorator
{
public:
                    ActiveTest (CppUnit::Test *test);
                    ~ActiveTest ();

    void            run (CppUnit::TestResult *result);

protected:
    HANDLE          m_threadHandle;
    CEvent          m_runCompleted;
    CppUnit::TestResult      *m_currentTestResult;

    void            run ();
    void            setTestResult (CppUnit::TestResult *result);
    static UINT     threadFunction (LPVOID thisInstance);


};


// Construct the active test
inline ActiveTest::ActiveTest (CppUnit::Test *test)
: TestDecorator (test), m_runCompleted () 
{ m_currentTestResult = NULL; m_threadHandle = INVALID_HANDLE_VALUE; }


// Pend until the test has completed
inline ActiveTest::~ActiveTest ()
{ CSingleLock (&m_runCompleted, TRUE); }


// Set the test result that we are to run
inline void ActiveTest::setTestResult (CppUnit::TestResult *result)
{ m_currentTestResult = result; }

// Run our test result
inline void ActiveTest::run ()
{ TestDecorator::run (m_currentTestResult); }

#endif


