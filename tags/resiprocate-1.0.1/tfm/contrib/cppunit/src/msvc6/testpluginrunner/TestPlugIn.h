// //////////////////////////////////////////////////////////////////////////
// Header file TestPlugIn.h for class TestPlugIn
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/06/23
// //////////////////////////////////////////////////////////////////////////
#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

#include <string>
#include <cppunit/test.h>
#include <msvc6/testrunner/TestPlugInInterface.h>


/*! \class TestPlugIn
 * \brief This class represents a test plug-in.
 */
class TestPlugIn
{
public:
  /*! Constructs a TestPlugIn object.
   */
  TestPlugIn( const std::string fileName );

  /*! Destructor.
   */
  virtual ~TestPlugIn();

  /*! Obtains a new test from a new copy of the dll.
   * \exception TestPlugInException if a error occurs.
   */
  CppUnit::Test *makeTest();

private:
  /// Prevents the use of the copy constructor.
  TestPlugIn( const TestPlugIn &copy );

  /// Prevents the use of the copy operator.
  void operator =( const TestPlugIn &copy );

  void reloadDll();
  void releaseDll();
  void deleteDllCopy();

  /*! Copy m_fileName DLL to m_copyFileName.
   *
   * Working on a copy of the DLL allow to update the original DLL.
   * \exception TestPlugInException on copy failure.
   */
  void makeDllCopy();

  /*! Load the DLL.
   * \exception TestPlugInException on dll loading failure.
   */
  void loadDll();

  /*! Get a pointer on the function "GetTestPlugInInterface" of the DLL.
   * \exception TestPlugInException if the function does not exist.
   */
  void getDllInterface();

private:
  std::string m_fileName;
  std::string m_copyFileName;
  HINSTANCE m_dllHandle;
  GetTestPlugInInterfaceFunction m_interfaceFunction;
};



// Inlines methods for TestPlugIn:
// -------------------------------



#endif  // TESTPLUGIN_H
