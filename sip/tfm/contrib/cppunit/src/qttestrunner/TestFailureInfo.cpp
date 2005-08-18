// //////////////////////////////////////////////////////////////////////////
// Implementation file TestFailureInfo.cpp for class TestFailureInfo
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/20
// //////////////////////////////////////////////////////////////////////////

#include "TestFailureInfo.h"
#include <cppunit/Exception.h>


TestFailureInfo::TestFailureInfo( CppUnit::Test *failedTest, 
                                  CppUnit::Exception *thrownException,
                                  bool isError ) : 
    CppUnit::TestFailure( failedTest, thrownException->clone(), isError )
{
}


TestFailureInfo::~TestFailureInfo()
{
}
