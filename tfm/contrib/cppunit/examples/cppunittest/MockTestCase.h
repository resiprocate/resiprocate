#ifndef MOCKTESTCASE_H
#define MOCKTESTCASE_H

#include <cppunit/TestCase.h>


/*! \class MockTestCase
 * \brief This class represents a mock test case.
 */
class MockTestCase : public CppUnit::TestCase
{
public:
  typedef CppUnit::TestCase SuperClass;   // work around VC++ call to super class method.

  /*! Constructs a MockTestCase object.
   */
  MockTestCase( std::string name );

  /// Destructor.
  virtual ~MockTestCase();

  void setExpectedSetUpCall( int callCount = 1 );
  void setExpectedTearDownCall( int callCount = 1 );
  void setExpectedRunTestCall( int callCount = 1 );
  void setExpectedCountTestCasesCall( int callCount = 1 );
  
  void makeSetUpThrow();
  void makeTearDownThrow();
  void makeRunTestThrow();
  
  void verify();

protected:
  int countTestCases() const;
  void setUp();
  void tearDown();
  void runTest();

private:
  /// Prevents the use of the copy constructor.
  MockTestCase( const MockTestCase &copy );

  /// Prevents the use of the copy operator.
  void operator =( const MockTestCase &copy );

private:
  bool m_hasSetUpExpectation;
  int m_expectedSetUpCall;
  int m_actualSetUpCall;

  bool m_hasTearDownExpectation;
  int m_expectedTearDownCall;
  int m_actualTearDownCall;

  bool m_expectRunTestCall;
  int m_expectedRunTestCallCount;
  int m_actualRunTestCallCount;
  bool m_expectCountTestCasesCall;
  int m_expectedCountTestCasesCallCount;
  mutable int m_actualCountTestCasesCallCount;

  bool m_setUpThrow;
  bool m_tearDownThrow;
  bool m_runTestThrow;
};





#endif  // MOCKTESTCASE_H
