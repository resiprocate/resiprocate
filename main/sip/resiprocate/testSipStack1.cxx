#include <iostream>
#ifndef WIN32
#include <unistd.h>
#endif

#include <sipstack/SipStack.hxx>
#include <sipstack/Logger.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main()
{
  SipStack sipStack;
  SipMessage* msg=NULL;

 while (1)
  {
    sipStack.process();

    msg = sipStack.receive();
    if ( msg )
    {
       DebugLog ( << "got message: " << *msg);
    }

    usleep( 50*1000); // sleep for 20 ms
  }
}
