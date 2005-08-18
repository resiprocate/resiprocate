// //////////////////////////////////////////////////////////////////////////
// Implementation file TestPlugIn.cpp for class TestPlugIn
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/06/23
// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestPlugIn.h"
#include <cppunit/TestCase.h>
#include "TestPlugInException.h"


TestPlugIn::TestPlugIn( const std::string fileName ) :
    m_fileName( fileName ),
    m_dllHandle( NULL ),
    m_interfaceFunction( NULL )
{
  m_copyFileName = m_fileName + "-hotrunner";
}


TestPlugIn::~TestPlugIn()
{
  releaseDll();
  deleteDllCopy();
}


void 
TestPlugIn::releaseDll()
{
  if ( m_dllHandle != NULL )
    ::FreeLibrary( m_dllHandle );
  m_dllHandle = NULL;
  m_interfaceFunction = NULL;
}


void 
TestPlugIn::deleteDllCopy()
{
  ::DeleteFile( m_copyFileName.c_str() );
}


class NullTest : public CppUnit::TestCase
{
public:
  NullTest( std::string name ) : TestCase( name ) 
  {
  }

  ~NullTest() 
  {
  }

  void runTests()
  {
    CPPUNIT_ASSERT_MESSAGE( "Failed to load" + getName(), FALSE );
  }
};


CppUnit::Test *
TestPlugIn::makeTest()
{
  reloadDll();
  TestPlugInInterface *theInterface = (*m_interfaceFunction)();

  try
  {
    return theInterface->makeTest();
  }
  catch ( ... )
  {
    throw TestPlugInException( "Failed to make test using GetTestPlugInInterface", 
                               TestPlugInException::failedToMakeTest );
  }
}


void 
TestPlugIn::reloadDll()
{
  releaseDll();
  makeDllCopy();
  loadDll();
  getDllInterface();
}


void 
TestPlugIn::makeDllCopy()
{
  if ( ::CopyFile( m_fileName.c_str(), m_copyFileName.c_str(), FALSE ) == FALSE )
  {
    throw TestPlugInException( "Failed to copy DLL" + m_fileName +
        " to " + m_copyFileName, TestPlugInException::failedToCopyDll );
  }
}


void 
TestPlugIn::loadDll()
{
  m_dllHandle = ::LoadLibrary( m_copyFileName.c_str() );
  if ( m_dllHandle == NULL )
    throw TestPlugInException( "Failed to load DLL " + m_copyFileName, 
                               TestPlugInException::failedToLoadDll );
}


void
TestPlugIn::getDllInterface()
{
  m_interfaceFunction = (GetTestPlugInInterfaceFunction )
      ::GetProcAddress( m_dllHandle, "GetTestPlugInInterface" );
  if ( m_interfaceFunction == NULL )
    throw TestPlugInException( "Failed to locate function GetTestPlugInInterface "
                               " in DLL " + m_fileName, 
                               TestPlugInException::failedToGetInterfaceFunction );
}
