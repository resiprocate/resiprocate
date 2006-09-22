// //////////////////////////////////////////////////////////////////////////
// Implementation file TestRunner.cpp for class TestRunner
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/19
// //////////////////////////////////////////////////////////////////////////

#include <qapplication.h>
#include <cppunit/TestSuite.h>
#include <cppunit/ui/qt/TestRunner.h>
#include "TestRunnerDlgImpl.h"
#include "TestRunnerModel.h"


namespace CppUnit
{
  namespace QtUi
  {


TestRunner::TestRunner() :
  _suite( new CppUnit::TestSuite( "All Tests" ) ),
  _tests( new Tests() )
{
}


TestRunner::~TestRunner()
{
  delete _suite;

  Tests::iterator it = _tests->begin();
  while ( it != _tests->end() )
    delete *it++;

  delete _tests;
}


Test *
TestRunner::getRootTest()
{
  if ( _tests->size() != 1 )
  {
    Tests::iterator it = _tests->begin();
    while ( it != _tests->end() )
      _suite->addTest( *it++ );
    _tests->clear();
    return _suite;
  }
  return (*_tests)[0];
}


void 
TestRunner::run( bool autoRun )
{
  TestRunnerDlg *dlg = new TestRunnerDlg( qApp->mainWidget(), 
                                          "TestRunner", 
                                          TRUE );
  dlg->setModel( new TestRunnerModel( getRootTest() ),
                 autoRun );
  dlg->exec();
  delete dlg;
}


void 
TestRunner::addTest( CppUnit::Test *test )
{
  _tests->push_back( test );
}


  }  // namespace QtUi
}  // namespace CppUnit
