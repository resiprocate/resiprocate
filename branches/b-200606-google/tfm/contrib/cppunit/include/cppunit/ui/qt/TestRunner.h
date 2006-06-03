// //////////////////////////////////////////////////////////////////////////
// Header file TestRunner.h for class TestRunner
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/19
// //////////////////////////////////////////////////////////////////////////
#ifndef CPPUNIT_QTUI_TESTRUNNER_H
#define CPPUNIT_QTUI_TESTRUNNER_H

#include <vector>
#include "Config.h"

namespace CppUnit
{
  class Test;
  class TestSuite;

  namespace QtUi
  {

/*! 
 * \brief QT test runner.
 * \ingroup ExecutingTest
 *
 * Here is an example of usage:
 * \code
 * #include <cppunit/extensions/TestFactoryRegistry.h>
 * #include <cppunit/ui/qt/TestRunner.h>
 *
 * [...]
 *
 * void 
 * QDepWindow::runTests()
 * {
 *   CppUnit::QtUi::TestRunner runner;
 *   runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
 *   runner.run( true );
 * }
 * \endcode
 *
 */
class QTTESTRUNNER_API TestRunner
{
public:
  /*! Constructs a TestRunner object.
   */
  TestRunner();

  /*! Destructor.
   */
  virtual ~TestRunner();

  void run( bool autoRun =false );

  void addTest( CppUnit::Test *test );

private:
  /// Prevents the use of the copy constructor.
  TestRunner( const TestRunner &copy );

  /// Prevents the use of the copy operator.
  void operator =( const TestRunner &copy );

  Test *getRootTest();

private:
  typedef std::vector<Test *> Tests;
  Tests *_tests;

  TestSuite *_suite;
};



// Inlines methods for TestRunner:
// -------------------------------


  }  // namespace QtUi
}  // namespace CppUnit

#endif  // CPPUNIT_QTUI_TESTRUNNER_H
