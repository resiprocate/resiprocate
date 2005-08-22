

#include "stdafx.h"
#include "ActiveTest.h"


// Spawn a thread to a test
void ActiveTest::run (CppUnit::TestResult *result)
{
    CWinThread *thread;
    
    setTestResult (result);
    m_runCompleted.ResetEvent ();

    thread = AfxBeginThread (threadFunction, 
        this, 
        THREAD_PRIORITY_NORMAL, 
        0, 
        CREATE_SUSPENDED);
    
    DuplicateHandle (GetCurrentProcess (), 
        thread->m_hThread,
        GetCurrentProcess (), 
        &m_threadHandle, 
        0, 
        FALSE, 
        DUPLICATE_SAME_ACCESS);

    thread->ResumeThread ();

}


// Simple execution thread.  Assuming that an ActiveTest instance
// only creates one of these at a time.
UINT ActiveTest::threadFunction (LPVOID thisInstance)
{
    ActiveTest *test = (ActiveTest *)thisInstance;

    test->run ();
    test->m_runCompleted.SetEvent ();

    return 0;
}

