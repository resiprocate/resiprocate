#ifndef CPPUNITTESTSUITE_H
#define CPPUNITTESTSUITE_H

#include <cppunit/TestSuite.h>


/*! \class CppUnitTestSuite
 * \brief This class is a test suite that includes all CppUnit unit tests.
 */
namespace CppUnitTest
{
  CppUnit::Test *suite();
} // namespace CppUnitTest


#endif  // CPPUNITTESTSUITE_H
