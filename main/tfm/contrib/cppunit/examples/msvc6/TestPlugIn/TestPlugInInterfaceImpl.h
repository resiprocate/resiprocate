// //////////////////////////////////////////////////////////////////////////
// Header file TestPlugInInterfaceImpl.h for class TestPlugInInterfaceImpl
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/06/27
// //////////////////////////////////////////////////////////////////////////
#ifndef TESTPLUGININTERFACEIMPL_H
#define TESTPLUGININTERFACEIMPL_H

#include "TestPlugIn.h"
#include <msvc6/testrunner/TestPlugInInterface.h>


/*! \class TestPlugInInterfaceImpl
 * \brief This class implements the test plug-in interface.
 */
class TestPlugInInterfaceImpl : public TestPlugInInterface
{
public:
  /*! Constructs a TestPlugInInterfaceImpl object.
   */
  TestPlugInInterfaceImpl();

  /*! Destructor.
   */
  virtual ~TestPlugInInterfaceImpl();


  CppUnit::Test *makeTest();

private:
  /// Prevents the use of the copy constructor.
  TestPlugInInterfaceImpl( const TestPlugInInterfaceImpl &copy );

  /// Prevents the use of the copy operator.
  void operator =( const TestPlugInInterfaceImpl &copy );

private:
};



extern "C" {
  TESTPLUGIN_API TestPlugInInterface *GetTestPlugInInterface();
}


#endif  // TESTPLUGININTERFACEIMPL_H
