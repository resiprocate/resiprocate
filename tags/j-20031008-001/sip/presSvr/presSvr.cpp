#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/Logger.hxx"
#include "PresConfig.h"
#include "TuPresSvr.h"


#include <time.h>

using namespace resip;
using namespace std;

int
main (int argc, char* argv[])
{
  Log::initialize(Log::COUT,Log::DEBUG,Data("PresSvr"));

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
