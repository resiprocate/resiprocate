// //////////////////////////////////////////////////////////////////////////
// Implementation file TestPlugInInterfaceImpl.cpp for class TestPlugInInterfaceImpl
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/06/27
// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestPlugInInterfaceImpl.h"
#include <cppunit/extensions/TestFactoryRegistry.h>


TestPlugInInterfaceImpl::TestPlugInInterfaceImpl()
{
}


TestPlugInInterfaceImpl::~TestPlugInInterfaceImpl()
{
}


CppUnit::Test *
TestPlugInInterfaceImpl::makeTest()
{
  return CppUnit::TestFactoryRegistry::getRegistry().makeTest();
}


TestPlugInInterface *
GetTestPlugInInterface()
{
  static TestPlugInInterfaceImpl runner;
  return &runner;
}

