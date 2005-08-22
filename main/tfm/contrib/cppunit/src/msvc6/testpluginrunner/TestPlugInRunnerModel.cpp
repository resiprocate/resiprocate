// //////////////////////////////////////////////////////////////////////////
// Implementation file TestPlugInRunnerModel.cpp for class TestPlugInRunnerModel
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/06/24
// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestPlugInRunnerModel.h"
#include <cppunit/TestSuite.h>
#include "TestPlugIn.h"


TestPlugInRunnerModel::TestPlugInRunnerModel() : 
    TestRunnerModel( new CppUnit::TestSuite( "Default" ) ),
    m_plugIn( new TestPlugIn( "default plug-in" ) )
{
}


TestPlugInRunnerModel::~TestPlugInRunnerModel()
{
  delete m_plugIn;
}


void 
TestPlugInRunnerModel::setPlugIn( TestPlugIn *plugIn )
{
  delete m_plugIn;
  m_plugIn = plugIn;
  reloadPlugIn();
}


void 
TestPlugInRunnerModel::reloadPlugIn()
{
  try 
  {
    setRootTest( m_plugIn->makeTest() );
  }
  catch (...)
  {
    setRootTest( new CppUnit::TestSuite( "Default" ) );  
    loadHistory();
    throw;
  }
}
