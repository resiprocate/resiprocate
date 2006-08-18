// //////////////////////////////////////////////////////////////////////////
// Header file TestFailureInfo.h for class TestFailureInfo
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/20
// //////////////////////////////////////////////////////////////////////////
#ifndef TESTFAILUREINFO_H
#define TESTFAILUREINFO_H

#include <cppunit/TestFailure.h>


/*! \class TestFailureInfo
 * \brief This class represents a test failure.
 */
class TestFailureInfo : public CppUnit::TestFailure
{
public:
  /*! Constructs a TestFailureInfo object.
   */
  TestFailureInfo( CppUnit::Test *failedTest, 
                   CppUnit::Exception *thrownException,
                   bool isError );

  /*! Destructor.
   */
  virtual ~TestFailureInfo();

private:
  /// Prevents the use of the copy constructor.
  TestFailureInfo( const TestFailureInfo &copy );

  /// Prevents the use of the copy operator.
  void operator =( const TestFailureInfo &copy );
};



// Inlines methods for TestFailureInfo:
// ------------------------------------



#endif  // TESTFAILUREINFO_H
