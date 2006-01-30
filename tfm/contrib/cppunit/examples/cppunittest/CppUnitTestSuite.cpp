#include <cppunit/extensions/TestFactoryRegistry.h>
#include "CppUnitTestSuite.h"
#include "CoreSuite.h"
#include "HelperSuite.h"
#include "ExtensionSuite.h"
#include "OutputSuite.h"
#include "UnitTestToolSuite.h"


namespace CppUnitTest
{

CppUnit::Test *
suite()
{
  CppUnit::TestFactoryRegistry &registry = 
                      CppUnit::TestFactoryRegistry::getRegistry();

  registry.registerFactory( 
      &CppUnit::TestFactoryRegistry::getRegistry( coreSuiteName() ) );
  registry.registerFactory( 
      &CppUnit::TestFactoryRegistry::getRegistry( extensionSuiteName() ) );
  registry.registerFactory( 
      &CppUnit::TestFactoryRegistry::getRegistry( helperSuiteName() ) );
  registry.registerFactory( 
      &CppUnit::TestFactoryRegistry::getRegistry( outputSuiteName() ) );
  registry.registerFactory( 
      &CppUnit::TestFactoryRegistry::getRegistry( unitTestToolSuiteName() ) );

  return registry.makeTest();
}


}  // namespace CppUnitTest
