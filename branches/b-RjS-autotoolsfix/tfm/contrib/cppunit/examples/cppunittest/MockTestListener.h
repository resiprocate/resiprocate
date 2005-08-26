#ifndef MOCKTESTLISTENER_H
#define MOCKTESTLISTENER_H

#include <cppunit/TestListener.h>
#include <string>


class MockTestListener : public CppUnit::TestListener
{
public:
  MockTestListener( std::string name );
  virtual ~MockTestListener() {}

  void setExpectFailure( CppUnit::Test *failedTest,
                         CppUnit::Exception *thrownException,
                         bool isError );
  void setExpectNoFailure();
  void setExpectFailure();
  void setExpectedAddFailureCall( int callCount );
  void setExpectStartTest( CppUnit::Test *test );
  void setExpectedStartTestCall( int callCount );
  void setExpectEndTest( CppUnit::Test *test );
  void setExpectedEndTestCall( int callCount );

  void addFailure( const CppUnit::TestFailure &failure );
  void startTest( CppUnit::Test *test );
  void endTest( CppUnit::Test *test );

  void verify();

private:
  std::string m_name;

  bool m_hasExpectationForStartTest;
  bool m_hasParametersExpectationForStartTest;
  int m_expectedStartTestCallCount;
  int m_startTestCall;
  CppUnit::Test *m_expectedStartTest;

  bool m_hasExpectationForEndTest;
  bool m_hasParametersExpectationForEndTest;
  int m_expectedEndTestCallCount;
  CppUnit::Test *m_expectedEndTest;
  int m_endTestCall;

  bool m_hasExpectationForAddFailure;
  bool m_hasExpectationForSomeFailure;
  bool m_hasParametersExpectationForAddFailure;
  int m_expectedAddFailureCallCount;
  int m_addFailureCall;
  CppUnit::Test *m_expectedFailedTest;
  CppUnit::Exception *m_expectedException;
  bool m_expectedIsError;
};



// Inlines methods for MockTestListener:
// -------------------------------------



#endif  // MOCKTESTLISTENER_H
