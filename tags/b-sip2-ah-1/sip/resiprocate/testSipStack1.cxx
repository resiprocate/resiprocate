#include <sipstack/SipStack.hxx>
#include <iostream>
#include <unistd.h>

using namespace Vocal2;
using namespace std;

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
      cout << msg << endl;
    }

    usleep( 50*1000); // sleep for 20 ms
  }
}
