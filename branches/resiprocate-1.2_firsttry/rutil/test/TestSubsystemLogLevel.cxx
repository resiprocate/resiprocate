
#include "rutil/Logger.hxx"
#include "TestSubsystemLogLevel.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

void 
Noisy::outputLogMessages()
{
   StackLog(<< "  STACK");
   DebugLog(<< "  DEBUG");
   InfoLog(<<  "  INFO");
   CritLog(<<  "  CRIT");
}
