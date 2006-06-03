#include "resip/stack/SipStack.hxx"
#include "rutil/Logger.hxx"
#include "PresConfig.h"
#include "TuPresSvr.h"


#include <time.h>

using namespace resip;
using namespace std;

int
main (int argc, char* argv[])
{
  Log::initialize(Log::Cout,Log::Debug,Data("PresSvr"));

  SipStack stack1;

  PresConfig::instance().initializeStack(stack1,argc,argv);
  PresConfig::instance().initializeResources(argc,argv);

  TuPresSvr tu(&stack1);

  bool done = 0;
  while(!done)
  {
    done = tu.process();
  }

  //this is bogus
  time_t start; time(&start);
  done = 0;
  while (!done)
  {
    tu.process();
    done = ((time(0)-start)>64);
  }
  
}
