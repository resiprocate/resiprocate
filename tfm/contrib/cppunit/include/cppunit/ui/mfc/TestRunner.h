#ifndef CPPUNITUI_MFC_TESTRUNNER_H
#define CPPUNITUI_MFC_TESTRUNNER_H


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <cppunit/Portability.h>
#include <vector>

/* Refer to MSDN documentation to know how to write and use MFC extension DLL:
   mk:@MSITStore:h:\DevStudio\MSDN\98VSa\1036\vcmfc.chm::/html/_mfcnotes_tn033.htm#_mfcnotes_how_to_write_an_mfc_extension_dll
   
   This can be found in the index with "mfc extension"
   The basic:
   Using:
   - your application must use MFC DLL
   - memory allocation is done using the same heap
   - you must define the symbol _AFX_DLL

   Building:
   - you must define the symbol _AFX_DLL and _AFX_EXT
   - export class using AFX_EXT_CLASS
 */

namespace CppUnit
{
  class Test;
  class TestSuite;

namespace MfcUi
{


/*! \brief MFC test runner.
 * \ingroup ExecutingTest
 *
 * Use this to launch the MFC TestRunner. Usually called from you CWinApp subclass:
 *
 * \code
 * #include <cppunit/ui/mfc/TestRunner.h>
 * #include <cppunit/extensions/TestFactoryRegistry.h>
 *
 * void 
 * CHostAppApp::RunUnitTests()
 * {
 *   CppUnit::MfcUi::TestRunner runner;
 *   runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
 *
 *   runner.run();    
 * }
 * \endcode
 * \see CppUnit::TextUi::TestRunner, CppUnit::TestFactoryRegistry.
 */
class AFX_EXT_CLASS TestRunner 
{
public:
  TestRunner();
  virtual ~TestRunner();

  void run();

  void addTest( CppUnit::Test *test );

  void addTests( const std::vector<CppUnit::Test *> &tests );

protected:
  CppUnit::Test *getRootTest();

  CppUnit::TestSuite *m_suite;

  typedef std::vector<CppUnit::Test *> Tests;
  Tests m_tests;
};


} // namespace MfcUi

} // namespace CppUnit


#endif  // CPPUNITUI_MFC_TESTRUNNER_H
