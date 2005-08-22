// //////////////////////////////////////////////////////////////////////////
// Implementation file TestRunner.cpp for class TestRunner
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/04/26
// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <cppunit/ui/mfc/TestRunner.h>
#include <cppunit/TestSuite.h>
#include "TestRunnerModel.h"
#include "TestRunnerDlg.h"



namespace CppUnit
{

namespace MfcUi
{


TestRunner::TestRunner() :
    m_suite( new CppUnit::TestSuite() )
{
}


TestRunner::~TestRunner() 
{
  delete m_suite;

  for ( Tests::iterator it = m_tests.begin(); it != m_tests.end(); ++it )
    delete *it;
}


void 
TestRunner::run() 
{ 
  bool comInit = SUCCEEDED( CoInitialize( NULL) );

  TestRunnerModel model( getRootTest() );
  TestRunnerDlg dlg( &model ); 
  dlg.DoModal (); 

  if ( comInit)
    CoUninitialize();
}


void            
TestRunner::addTest(CppUnit::Test *test) 
{ 
  m_tests.push_back( test );
}


void            
TestRunner::addTests ( const std::vector<CppUnit::Test *> &tests ) 
{ 
  for ( std::vector<CppUnit::Test *>::const_iterator it=tests.begin();
        it != tests.end();
        ++it )
  {
    addTest( *it );
  }
}


CppUnit::Test *   
TestRunner::getRootTest()
{
  if ( m_tests.size() != 1 )
  {
    for ( Tests::iterator it = m_tests.begin(); it != m_tests.end(); ++it )
      m_suite->addTest( *it );
    m_tests.clear();
    return m_suite;
  }
  return m_tests[0];
}

} // namespace MfcUi

} // namespace CppUnit
