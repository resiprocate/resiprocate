// //////////////////////////////////////////////////////////////////////////
// Implementation file TestRunnerModel.cpp for class TestRunnerModel
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/04/26
// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestRunnerModel.h"
#include <algorithm>
#include <cppunit/testsuite.h>


TestRunnerModel::TestRunnerModel( CppUnit::Test *rootTest ) :
    m_rootTest( rootTest )
{
}


TestRunnerModel::~TestRunnerModel()
{
}

const TestRunnerModel::History &
TestRunnerModel::history() const
{
  return m_history;
}


void 
TestRunnerModel::selectHistoryTest( CppUnit::Test *test )
{
  History::iterator end = 
      std::remove( m_history.begin(), m_history.end(), test );
  m_history.erase( end, m_history.end() );
  
  if ( test != NULL )
    m_history.push_front( test );
}


CppUnit::Test *
TestRunnerModel::selectedTest() const
{
  if ( m_history.size() > 0 )
    return m_history[0];
  return NULL;
}


void 
TestRunnerModel::loadSettings(Settings & s)
{
  CWinApp *app = AfxGetApp();
  ASSERT( app != NULL );

  int autorun = app->GetProfileInt( _T("CppUnit"),
                                    _T("AutorunAtStartup"),
                                    1 );
  s.autorunOnLaunch = (autorun == 1);

  s.dlgBounds.left = app->GetProfileInt( _T("CppUnit"), _T("Left"), 0 );
  s.dlgBounds.top = app->GetProfileInt( _T("CppUnit"), _T("Top"), 0 );
  s.dlgBounds.right = app->GetProfileInt( _T("CppUnit"), _T("Right"), 0 );
  s.dlgBounds.bottom= app->GetProfileInt( _T("CppUnit"), _T("Bottom"), 0 );  

  s.col_1 = app->GetProfileInt( _T("CppUnit"), _T("Col_1"), 40 );
  s.col_2 = app->GetProfileInt( _T("CppUnit"), _T("Col_2"), 40 );
  s.col_3 = app->GetProfileInt( _T("CppUnit"), _T("Col_3"), 40 );
  s.col_4 = app->GetProfileInt( _T("CppUnit"), _T("Col_4"), 40 );

  loadHistory();
}


void 
TestRunnerModel::loadHistory()
{
  m_history.clear();
  int idx = 1;
  do
  {
    CString testName = loadHistoryEntry( idx++ );
    if ( testName.IsEmpty() )
      break;

    CppUnit::Test *test = findTestByName( testName );
    if ( test != NULL )
      m_history.push_back( test );
  }
  while ( true );
}


CString
TestRunnerModel::loadHistoryEntry( int idx )
{
  CWinApp *app = AfxGetApp();
  ASSERT( app != NULL );

  return app->GetProfileString( _T("CppUnit"), getHistoryEntryName( idx ) );
}


void 
TestRunnerModel::saveSettings( const Settings & s )
{
  CWinApp *app = AfxGetApp();
  ASSERT( app != NULL );

  int autorun = s.autorunOnLaunch ? 1 : 0;
  app->WriteProfileInt( _T("CppUnit"), _T("AutorunAtStartup"), autorun );

  app->WriteProfileInt( _T("CppUnit"), _T("Left"),	 s.dlgBounds.left );
  app->WriteProfileInt( _T("CppUnit"), _T("Top"),	 s.dlgBounds.top );
  app->WriteProfileInt( _T("CppUnit"), _T("Right"),  s.dlgBounds.right );
  app->WriteProfileInt( _T("CppUnit"), _T("Bottom"), s.dlgBounds.bottom );

  app->WriteProfileInt( _T("CppUnit"), _T("Col_1"),	 s.col_1 );
  app->WriteProfileInt( _T("CppUnit"), _T("Col_2"),	 s.col_2 );
  app->WriteProfileInt( _T("CppUnit"), _T("Col_3"),	 s.col_3 );
  app->WriteProfileInt( _T("CppUnit"), _T("Col_4"),	 s.col_4 );

  int idx = 1;
  for ( History::const_iterator it = m_history.begin(); 
        it != m_history.end(); 
        ++it , ++idx )
  {
    CppUnit::Test *test = *it;
    saveHistoryEntry( idx, test->getName().c_str() );
  }
}


void 
TestRunnerModel::saveHistoryEntry( int idx, 
                                   CString testName )
{
  CWinApp *app = AfxGetApp();
  ASSERT( app != NULL );

  app->WriteProfileString( _T("CppUnit"),
                           getHistoryEntryName( idx ),
                           testName );
}


CString
TestRunnerModel::getHistoryEntryName( int idx ) const
{
  CString entry;
  entry.Format( _T("HistoryTest%d"), idx );
  return entry;
}


CppUnit::Test *
TestRunnerModel::rootTest()
{
  return m_rootTest;
}


void 
TestRunnerModel::setRootTest( CppUnit::Test *test )
{
  m_rootTest = test;
}


CppUnit::Test * 
TestRunnerModel::findTestByName( CString name ) const
{
  return findTestByNameFor( name, m_rootTest );
}


CppUnit::Test * 
TestRunnerModel::findTestByNameFor( const CString &name, 
                                    CppUnit::Test *test ) const
{
  if ( name == test->getName().c_str() )
    return test;

  CppUnit::TestSuite *suite = dynamic_cast<CppUnit::TestSuite *>(test);
  if ( suite == NULL )
    return NULL;

  const std::vector<CppUnit::Test *> &tests = suite->getTests();
  for ( std::vector<CppUnit::Test *>::const_iterator it = tests.begin(); 
        it != tests.end(); 
        ++it )
  {
    CppUnit::Test *testFound = findTestByNameFor( name, *it );
    if ( testFound != NULL )
      return testFound;
  }
  return NULL;
}
