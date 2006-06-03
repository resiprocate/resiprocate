#include "cppunit/TextTestResult.h"
#include "cppunit/TestSuite.h"

#include "BoardGame.h"
#include "Chess.h"

#include "BoardGameTest.h"
#include "ChessTest.h"


#include <vector>
#include <iostream>



int 
main(int argc, char** argv)
{
  CppUnit::TestSuite suite;

  suite.addTest( BoardGameTest<BoardGame>::suite() );
  suite.addTest( ChessTest<Chess>::suite() );

  CppUnit::TextTestResult res;

  suite.run( &res );
  std::cout << res << std::endl;
  return 0;
}
